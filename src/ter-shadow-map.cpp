#include "ter-shadow-map.h"
#include "ter-shader-program.h"

#include <glib.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

TerShadowMap *
ter_shadow_map_new(int width, int height)
{
   TerShadowMap *m = g_new0(TerShadowMap, 1);
   m->map = ter_render_depth_texture_new(width, height, true);
   return m;
}

void
ter_shadow_map_free(TerShadowMap *sm)
{
   ter_render_texture_free(sm->map);
   g_free(sm);
}
