#ifndef __TER_RENDER_TEXTURE_H__
#define __TER_RENDER_TEXTURE_H__

typedef struct {
   int width, height;
   int prev_viewport[4];
   unsigned framebuffer;
   unsigned texture;
   unsigned depthbuffer;
   unsigned depth_texture;
   bool is_multisampled;
} TerRenderTexture;

TerRenderTexture *ter_render_texture_new(int width, int height,
                                         bool use_depth_texture,
                                         bool multisample);

TerRenderTexture *ter_render_depth_texture_new(int width, int height, bool is_shadow);

void ter_render_texture_free(TerRenderTexture *rt);

void ter_render_texture_start(TerRenderTexture *rt);
void ter_render_texture_stop(TerRenderTexture *rt);

void ter_render_texture_blit(TerRenderTexture *src, TerRenderTexture *dst);
void ter_render_texture_blit_to_window(TerRenderTexture *src,
                                       unsigned width, unsigned height);

#endif
