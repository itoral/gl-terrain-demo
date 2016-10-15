#ifndef __TER_SHADOW_RENDERER_H__
#define __TER_SHADOW_RENDERER_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glib.h>

#include "ter-shadow-map.h"
#include "ter-shadow-box.h"

typedef struct {
   TerLight *light;
   TerShadowBox *shadow_box;
   glm::mat4 LightProjection[TER_MAX_CSM_LEVELS];
   glm::mat4 LightView[TER_MAX_CSM_LEVELS];
} TerShadowRenderer;

TerShadowRenderer *ter_shadow_renderer_new(TerLight *light, TerCamera *cam);
void ter_shadow_renderer_free(TerShadowRenderer *sr);

glm::mat4 ter_shadow_renderer_get_shadow_map_space_vp(TerShadowRenderer *sr,
                                                      unsigned level);
bool ter_shadow_renderer_render(TerShadowRenderer *sr);

#endif
