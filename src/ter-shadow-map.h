#ifndef __TER_SHADOW_MAP_H__
#define __TER_SHADOW_MAP_H__

#include "ter-render-texture.h"

#include <glib.h>

typedef struct {
   TerRenderTexture *map;
} TerShadowMap;

TerShadowMap *ter_shadow_map_new(int width, int height);
void ter_shadow_map_free(TerShadowMap *sm);

#endif
