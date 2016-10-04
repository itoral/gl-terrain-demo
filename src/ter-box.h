#ifndef __TER_BOX_H__
#define __TER_BOX_H__

#include <glm/glm.hpp>

typedef struct {
   glm::vec3 center;
   float w, h, d;
} TerBox;

bool ter_box_is_inside(TerBox *box, glm::vec3 *p);
bool ter_box_collision(TerBox *box1, TerBox *box2);
void ter_box_transform(TerBox *box, glm::mat4 *transform);

#endif
