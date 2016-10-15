#ifndef __TER_SHADOW_BOX_H__
#define __TER_SHADOW_BOX_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ter-light.h"
#include "ter-camera.h"
#include "ter-shadow-map.h"

#define TER_MAX_CSM_LEVELS 4

typedef struct {
   float minX, maxX;
   float minY, maxY;
   float minZ, maxZ;

   float far_dist;
   float near_dist;
   float near_width, near_height;
   float far_width, far_height;

   glm::vec3 frustum[8];
   TerShadowMap *shadow_map;
} TerShadowBoxLevel;

typedef struct {
   TerShadowBoxLevel csm[TER_MAX_CSM_LEVELS];
   unsigned csm_levels;

   glm::mat4 light_view_matrix;
   TerCamera *camera;
} TerShadowBox;

TerShadowBox *ter_shadow_box_new(TerLight *light, TerCamera *camera);
void ter_shadow_box_free(TerShadowBox *sb);

void ter_shadow_box_update(TerShadowBox *sb);

float ter_shadow_box_get_width(TerShadowBox *sb, int l);
float ter_shadow_box_get_height(TerShadowBox *sb, int l);
float ter_shadow_box_get_depth(TerShadowBox *sb, int l);
glm::vec3 ter_shadow_box_get_center(TerShadowBox *sb, int l);
void ter_shadow_box_get_clipping_box(TerShadowBox *sb,
                                     glm::vec3 *c, float *w, float *h, float *d,
                                     int l);
TerShadowMap *ter_shadow_box_get_shadow_map(TerShadowBox *sb, int l);
float ter_shadow_box_get_far_distance(TerShadowBox *sb, int l);
float ter_shadow_box_get_map_size(TerShadowBox *sb, int l);

#endif
