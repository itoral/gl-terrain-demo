#include "ter-render-texture.h"
#include "ter-util.h"

#include <glib.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

TerRenderTexture *
ter_render_texture_new(int width, int height,
                       bool clamp_to_edge,
                       bool needs_depth,
                       bool use_depth_texture,
                       bool multisample)
{
   /* We don't use depth textures with multisampled fbos */
   assert(!multisample || !use_depth_texture);

   if (TER_MULTISAMPLING_SAMPLES <= 1)
      multisample = false;

   TerRenderTexture *rt = g_new0(TerRenderTexture, 1);
   rt->width = width;
   rt->height = height;
   rt->is_multisampled = multisample;

   glGenFramebuffers(1, &rt->framebuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, rt->framebuffer);
   glDrawBuffer(GL_COLOR_ATTACHMENT0);

   /* Color attachment */
   if (!rt->is_multisampled) {
      glGenTextures(1, &rt->texture);
      glBindTexture(GL_TEXTURE_2D, rt->texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      if (clamp_to_edge) {
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, rt->texture, 0);
   } else {
      glGenTextures(1, &rt->texture);
      glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, rt->texture);
      glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE,
                              TER_MULTISAMPLING_SAMPLES,
                              GL_RGBA8, width, height, true);
      if (clamp_to_edge) {
         glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE,
                         GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE,
                         GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D_MULTISAMPLE, rt->texture, 0);
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
         glBindTexture(GL_TEXTURE_2D, rt->depth_texture);
         glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0,
                      GL_DEPTH_COMPONENT, GL_FLOAT, 0);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
         if (clamp_to_edge) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
   glBindTexture(GL_TEXTURE_2D, rt->depth_texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0,
                GL_DEPTH_COMPONENT, GL_FLOAT, 0);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
      is_shadow ? GL_LINEAR : GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
         is_shadow ? GL_LINEAR : GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   if (is_shadow) {
	   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE,
	                   GL_COMPARE_R_TO_TEXTURE);
	   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
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
   if (rt->texture)
      glDeleteTextures(1, &rt->texture);
   if (rt->depth_texture)
      glDeleteTextures(1, &rt->depth_texture);
   if (rt->depthbuffer)
      glDeleteRenderbuffers(1, &rt->depthbuffer);
   g_free(rt);
}

void
ter_render_texture_start(TerRenderTexture *rt)
{
   glGetIntegerv(GL_VIEWPORT, rt->prev_viewport);
   glBindTexture(GL_TEXTURE_2D, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, rt->framebuffer);
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
   glBindFramebuffer(GL_READ_FRAMEBUFFER, src->framebuffer);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->framebuffer);
   glBlitFramebuffer(0, 0, src->width, src->height,
                     0, 0, dst->width, dst->height,
                     GL_COLOR_BUFFER_BIT |
                        (dst->has_depth ? GL_DEPTH_BUFFER_BIT : 0),
                     GL_NEAREST);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
ter_render_texture_blit_to_window(TerRenderTexture *src,
                                  unsigned width, unsigned height)
{
   glBindFramebuffer(GL_READ_FRAMEBUFFER, src->framebuffer);
   glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
   glDrawBuffer(GL_BACK);
   glBlitFramebuffer(0, 0, src->width, src->height,
                     0, 0, width, height,
                     GL_COLOR_BUFFER_BIT, GL_NEAREST);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
