#include "ter-filter.h"
#include "ter-shader-program.h"
#include "ter-cache.h"

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#include <glib.h>

TerBloomFilter *
ter_bloom_filter_new()
{
   TerBloomFilter *bf = g_new0(TerBloomFilter, 1);

   assert(TER_BLOOM_BLUR_SCALE > 0.0f && TER_BLOOM_BLUR_SCALE <= 1.0f);

   bf->brightness_fbo =
      ter_render_texture_new(TER_WIN_WIDTH, TER_WIN_HEIGHT,
                             true, false, false, false);
   bf->hblur_fbo =
      ter_render_texture_new(TER_WIN_WIDTH * TER_BLOOM_BLUR_SCALE,
                             TER_WIN_HEIGHT * TER_BLOOM_BLUR_SCALE,
                             true, false, false, false);
   bf->vblur_fbo =
      ter_render_texture_new(TER_WIN_WIDTH * TER_BLOOM_BLUR_SCALE,
                             TER_WIN_HEIGHT * TER_BLOOM_BLUR_SCALE,
                             true, false, false, false);
   bf->result_fbo =
      ter_render_texture_new(TER_WIN_WIDTH, TER_WIN_HEIGHT,
                             true, false, false, false);
   return bf;
}

static inline void
ter_postprocess_bind_vao()
{
   static unsigned vao = 0;
   static unsigned vertex_buf = 0;
   static unsigned uv_buf = 0;

   if (!vao) {
      glm::vec2 vertices[4] = {
         glm::vec2(-1.0f, -1.0f),
         glm::vec2( 1.0f, -1.0f),
         glm::vec2(-1.0f,  1.0f),
         glm::vec2( 1.0f,  1.0f)
      };

      glm::vec2 uvs[4] = {
         glm::vec2(0.0f, 0.0f),
         glm::vec2(1.0f, 0.0f),
         glm::vec2(0.0f, 1.0f),
         glm::vec2(1.0f, 1.0f)
      };

      glGenBuffers(1, &vertex_buf);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
      glBufferData(GL_ARRAY_BUFFER,
                   4 * sizeof(glm::vec2), &vertices[0], GL_STATIC_DRAW);

      glGenBuffers(1, &uv_buf);
      glBindBuffer(GL_ARRAY_BUFFER, uv_buf);
      glBufferData(GL_ARRAY_BUFFER,
                   4 * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);

      glGenVertexArrays(1, &vao);
      glBindVertexArray(vao);

      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
      glVertexAttribPointer(
         0,                  // Attribute index
         2,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );

      glEnableVertexAttribArray(1);
      glBindBuffer(GL_ARRAY_BUFFER, uv_buf);
      glVertexAttribPointer(
         1,                  // Attribute index
         2,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );
   } else {
      glBindVertexArray(vao);
   }
}

static void
ter_filter_simple_render(unsigned src_tex, unsigned src_sam, TerShaderProgramFilterSimple *sh)
 __attribute__ ((unused));

static void
ter_filter_simple_render(unsigned src_tex, unsigned src_sam, TerShaderProgramFilterSimple *sh)
{
   glUseProgram(sh->prog.program);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, src_tex);
   glBindSampler(0, src_sam);
   ter_shader_program_filter_simple_load(sh, 0);

   ter_postprocess_bind_vao();
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void
ter_filter_brightness_select_render(unsigned src_tex, unsigned src_sam, float lum_factor,
                                    TerShaderProgramFilterBrightnessSelect *sh)
{
   glUseProgram(sh->simple.prog.program);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, src_tex);
   glBindSampler(0, src_sam);
   ter_shader_program_filter_brightness_select_load(sh, 0, lum_factor);

   ter_postprocess_bind_vao();
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void
ter_filter_blur_render(unsigned src_tex, unsigned src_sam, unsigned dim,
                       TerShaderProgramFilterBlur *sh)
{
   glUseProgram(sh->simple.prog.program);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, src_tex);
   glBindSampler(0, src_sam);
   ter_shader_program_filter_blur_load(sh, 0, dim);

   ter_postprocess_bind_vao();
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void
ter_filter_combine_render(unsigned src1_tex, unsigned src1_sam,
                          unsigned src2_tex, unsigned src2_sam,
                          TerShaderProgramFilterCombine *sh)
{
   glUseProgram(sh->simple.prog.program);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, src1_tex);
   glBindSampler(0, src1_sam);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, src2_tex);
   glBindSampler(1, src2_sam);
   ter_shader_program_filter_combine_load(sh, 0, 1);

   ter_postprocess_bind_vao();
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

TerRenderTexture *
ter_bloom_filter_run(TerBloomFilter *bf, TerRenderTexture *src)
{
   static TerShaderProgramFilterBrightnessSelect *sh_brightness =
      (TerShaderProgramFilterBrightnessSelect *)
         ter_cache_get("program/bloom-brightness");
   static TerShaderProgramFilterBlur *sh_hblur =
      (TerShaderProgramFilterBlur *) ter_cache_get("program/bloom-hblur");
   static TerShaderProgramFilterBlur *sh_vblur =
      (TerShaderProgramFilterBlur *) ter_cache_get("program/bloom-vblur");
   static TerShaderProgramFilterCombine *sh_combine =
      (TerShaderProgramFilterCombine *) ter_cache_get("program/bloom-combine");

   ter_render_texture_start(bf->brightness_fbo);
   ter_filter_brightness_select_render(src->texture[0],
                                       src->sampler[0],
                                       TER_BLOOM_LUMINANCE_FACTOR,
                                       sh_brightness);
   ter_render_texture_stop(bf->brightness_fbo);

   ter_render_texture_start(bf->hblur_fbo);
   ter_filter_blur_render(bf->brightness_fbo->texture[0],
                          bf->brightness_fbo->sampler[0],
                          bf->hblur_fbo->width,
                          sh_hblur);
   ter_render_texture_stop(bf->hblur_fbo);

   ter_render_texture_start(bf->vblur_fbo);
   ter_filter_blur_render(bf->hblur_fbo->texture[0],
                          bf->hblur_fbo->sampler[0],
                          bf->vblur_fbo->height,
                          sh_vblur);
   ter_render_texture_stop(bf->vblur_fbo);

   ter_render_texture_start(bf->result_fbo);
   ter_filter_combine_render(src->texture[0], src->sampler[0],
                             bf->vblur_fbo->texture[0], bf->vblur_fbo->sampler[0],
                             sh_combine);
   ter_render_texture_stop(bf->result_fbo);

   return bf->result_fbo;
}

void
ter_bloom_filter_free(TerBloomFilter *bf)
{
   ter_render_texture_free(bf->brightness_fbo);
   ter_render_texture_free(bf->hblur_fbo);
   ter_render_texture_free(bf->vblur_fbo);
   ter_render_texture_free(bf->result_fbo);
   g_free(bf);
}

TerMotionBlurFilter *
ter_motion_blur_filter_new()
{
   TerMotionBlurFilter *f = g_new0(TerMotionBlurFilter, 1);

   f->result = ter_render_texture_new(TER_WIN_WIDTH, TER_WIN_HEIGHT,
                                      true, false, false, false);
   return f;
}

TerRenderTexture *
ter_motion_blur_filter_run(TerMotionBlurFilter *f, TerRenderTexture *src)
{
   static TerShaderProgramFilterMotionBlur *sh =
      (TerShaderProgramFilterMotionBlur *) ter_cache_get("program/motion-blur");

   ter_render_texture_start(f->result);

   glUseProgram(sh->simple.prog.program);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, src->texture[0]);
   glBindSampler(0, src->sampler[0]);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, src->texture[1]);
   glBindSampler(1, src->sampler[1]);
   ter_shader_program_filter_motion_blur_load(sh, 0, 1,
                                              TER_MOTION_BLUR_DIVISOR);

   ter_postprocess_bind_vao();
   glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

   ter_render_texture_stop(f->result);

   return f->result;
}

void
ter_motion_blur_filter_free(TerMotionBlurFilter *f)
{
   ter_render_texture_free(f->result);
   g_free(f);
}
