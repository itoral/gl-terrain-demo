#ifndef __TER_SKY_BOX_H__
#define __TER_SKY_BOX_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>

typedef struct {
   unsigned size;
   unsigned vtid;

   glm::vec3 rot;
   float speed;

   unsigned vao;
   unsigned vertex_buf;

   glm::mat4 prev_mvp;
   bool prev_mvp_valid;
} TerSkyBox;

TerSkyBox *ter_skybox_new(unsigned size, unsigned vtid);
void ter_skybox_free(TerSkyBox *b);

void ter_skybox_update(TerSkyBox *b);
void ter_skybox_render(TerSkyBox *b, bool render_motion);

#endif
