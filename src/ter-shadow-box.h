#ifndef __TER_SHADOW_BOX_H__
#define __TER_SHADOW_BOX_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ter-light.h"
#include "ter-camera.h"

typedef struct {
   float minX, maxX;
   float minY, maxY;
   float minZ, maxZ;

   float near_width, near_height;
   float far_width, far_height;

   glm::mat4 light_view_matrix;
   TerCamera *camera;

   glm::vec3 frustum[8];
} TerShadowBox;

TerShadowBox *ter_shadow_box_new(TerLight *light, TerCamera *camera);
void ter_shadow_box_free(TerShadowBox *sb);

void ter_shadow_box_update(TerShadowBox *sb);

float ter_shadow_box_get_width(TerShadowBox *sb);
float ter_shadow_box_get_height(TerShadowBox *sb);
float ter_shadow_box_get_depth(TerShadowBox *sb);
glm::vec3 ter_shadow_box_get_center(TerShadowBox *sb);
void ter_shadow_box_get_clipping_box(TerShadowBox *sb, glm::vec3 *c,
                                     float *w, float *h, float *d);


#endif
