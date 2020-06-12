#include "ter-render-texture.h"
#include "ter-util.h"

#include <glib.h>

TerRenderTexture *
ter_render_texture_new(int width, int height,
                       bool clamp_to_edge,
                       bool needs_depth,
                       bool use_depth_texture,
                       bool multisample,
                       unsigned num_color_attachments)
{
   /* We don't use depth textures with multisampled fbos */
   assert(!multisample || !use_depth_texture);
   assert(num_color_attachments <= 4);

   if (TER_MULTISAMPLING_SAMPLES <= 1)
      multisample = false;

   TerRenderTexture *rt = g_new0(TerRenderTexture, 1);
   rt->width = width;
   rt->height = height;
   rt->is_multisampled = multisample;
   rt->num_color_textures = num_color_attachments;

   glGenFramebuffers(1, &rt->framebuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, rt->framebuffer);

   /* Color attachments */
   glGenTextures(num_color_attachments, rt->texture);
   glGenSamplers(num_color_attachments, rt->sampler);
   for (unsigned i = 0; i < num_color_attachments; i++) {
      int attachment = GL_COLOR_ATTACHMENT0 + i;
      if (!rt->is_multisampled) {
         glBindTexture(GL_TEXTURE_2D, rt->texture[i]);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                      0, GL_RGBA, GL_FLOAT, 0);
         glSamplerParameteri(rt->sampler[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         glSamplerParameteri(rt->sampler[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         if (clamp_to_edge) {
            glSamplerParameteri(rt->sampler[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri(rt->sampler[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
         }
         glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
                                GL_TEXTURE_2D, rt->texture[i], 0);
      } else {
         glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, rt->texture[i]);
         glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                                 TER_MULTISAMPLING_SAMPLES,
                                 GL_RGBA8, width, height, true);
         if (clamp_to_edge) {
            glSamplerParameteri(rt->sampler[i],
                                GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri(rt->sampler[i],
                                GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
         }
         glFramebufferTexture2D(GL_FRAMEBUFFER, attachment,
                                GL_TEXTURE_2D_MULTISAMPLE, rt->texture[i], 0);
      }
   }

   /* Depth attachment */
   if (needs_depth) {
      if (!use_depth_texture) {
         glGenRenderbuffers(1, &rt->depthbuffer);
         glBindRenderbuffer(GL_RENDERBUFFER, rt->depthbuffer);
         if (!rt->is_multisampled) {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                                  width, height);
         } else {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                                             TER_MULTISAMPLING_SAMPLES,
                                             GL_DEPTH_COMPONENT, width, height);
         }
         glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, rt->depthbuffer);
      } else {
         glGenTextures(1, &rt->depth_texture);
         glGenSamplers(1, &rt->depth_sampler);
         glBindTexture(GL_TEXTURE_2D, rt->depth_texture);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width, height, 0,
                      GL_DEPTH_COMPONENT, GL_FLOAT, 0);
         glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         if (clamp_to_edge) {
            glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
         }
         glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              rt->depth_texture, 0);
      }
   }

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      printf("ERROR: can't create framebuffer\n");
      exit(1);
   }

   glBindTexture(GL_TEXTURE_2D, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return rt;
}

TerRenderTexture *
ter_render_depth_texture_new(int width, int height, bool is_shadow)
{
   TerRenderTexture *rt = g_new0(TerRenderTexture, 1);
   rt->width = width;
   rt->height = height;

   glGenFramebuffers(1, &rt->framebuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, rt->framebuffer);
   glDrawBuffer(GL_NONE);

   glGenTextures(1, &rt->depth_texture);
   glGenSamplers(1, &rt->depth_sampler);
   glBindTexture(GL_TEXTURE_2D, rt->depth_texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0,
                GL_DEPTH_COMPONENT, GL_FLOAT, 0);
   glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_MAG_FILTER,
      is_shadow ? GL_LINEAR : GL_NEAREST);
   glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_MIN_FILTER,
         is_shadow ? GL_LINEAR : GL_NEAREST);
   glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   if (is_shadow) {
	   glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_COMPARE_MODE,
	                   GL_COMPARE_R_TO_TEXTURE);
	   glSamplerParameteri(rt->depth_sampler, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
   }

   glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                        rt->depth_texture, 0);

   if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      printf("ERROR: can't create framebuffer\n");
      exit(1);
   }

   glBindTexture(GL_TEXTURE_2D, 0);
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   return rt;
}

void
ter_render_texture_free(TerRenderTexture *rt)
{
   glDeleteFramebuffers(1, &rt->framebuffer);
   for (unsigned i = 0; i < rt->num_color_textures; i++) {
      glDeleteTextures(1, &rt->texture[i]);
      glDeleteSamplers(1, &rt->sampler[i]);
   }
   if (rt->depth_texture) {
      glDeleteTextures(1, &rt->depth_texture);
      glDeleteSamplers(1, &rt->depth_sampler);
   }
   if (rt->depthbuffer)
      glDeleteRenderbuffers(1, &rt->depthbuffer);
   g_free(rt);
}

void
ter_render_texture_start(TerRenderTexture *rt)
{
   static const GLenum color_attachments[] = {
      GL_COLOR_ATTACHMENT0,
      GL_COLOR_ATTACHMENT1,
      GL_COLOR_ATTACHMENT2,
      GL_COLOR_ATTACHMENT3,
   };

   glGetIntegerv(GL_VIEWPORT, rt->prev_viewport);
   glBindTexture(GL_TEXTURE_2D, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, rt->framebuffer);
   glDrawBuffers(rt->num_color_textures, color_attachments);
   glViewport(0, 0, rt->width, rt->height);
}

void
ter_render_texture_stop(TerRenderTexture *rt)
{
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glViewport(rt->prev_viewport[0], rt->prev_viewport[1],
              rt->prev_viewport[2], rt->prev_viewport[3]);
}

void
ter_render_texture_blit(TerRenderTexture *src,
                        TerRenderTexture *dst)
{
   static const GLenum color_attachments[] = {
      GL_COLOR_ATTACHMENT0,
      GL_COLOR_ATTACHMENT1,
      GL_COLOR_ATTACHMENT2,
      GL_COLOR_ATTACHMENT3,
   };

   glBindFramebuffer(GL_READ_FRAMEBUFFER, src->framebuffer);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->framebuffer);

   for (unsigned i = 0; i < src->num_color_textures; i++) {
      glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
      glDrawBuffers(1, &color_attachments[i]);

      glBlitFramebuffer(0, 0, src->width, src->height,
                        0, 0, dst->width, dst->height,
                        GL_COLOR_BUFFER_BIT |
                           (dst->has_depth ? GL_DEPTH_BUFFER_BIT : 0),
                        GL_NEAREST);
   }

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
ter_render_texture_blit_to_window(TerRenderTexture *src,
                                  unsigned width, unsigned height,
                                  unsigned attachment)
{
   glBindFramebuffer(GL_READ_FRAMEBUFFER, src->framebuffer);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glDrawBuffer(GL_BACK);
   glReadBuffer(attachment);
   glBlitFramebuffer(0, 0, src->width, src->height,
                     0, 0, width, height,
                     GL_COLOR_BUFFER_BIT, GL_NEAREST);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
