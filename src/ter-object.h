#ifndef __TER_OBJECT_H__
#define __TER_OBJECT_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ter-model.h"
#include "ter-box.h"

typedef struct {
   glm::vec3 pos;
   glm::vec3 rot;
   glm::vec3 scale;
   TerModel *model;
   int variant;
   bool cast_shadow;
   bool can_collide;
   TerBox box;
   glm::mat4 prev_mvp;
   bool prev_mvp_valid;
} TerObject;

TerObject *ter_object_new(TerModel *model, float x, float y, float z);
void ter_object_free(TerObject *o);

void ter_object_render(TerObject *o, bool enable_shadow);
glm::mat4 ter_object_get_model_matrix(TerObject *o);

float ter_object_get_height(TerObject *o);
float ter_object_get_width(TerObject *o);
float ter_object_get_depth(TerObject *o);

glm::vec3 ter_object_get_position(TerObject *o);

void ter_object_update_box(TerObject *o);
TerBox *ter_object_get_box(TerObject *o);

bool ter_object_collision(TerObject *o, TerBox *box);
glm::mat4 ter_object_get_model_matrix_for_box(TerObject *o);
bool ter_object_is_rotated(TerObject *o);

void ter_object_set_prev_mvp(TerObject *o, glm::mat4 mvp);

#endif
