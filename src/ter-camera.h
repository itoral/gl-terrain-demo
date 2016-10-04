#ifndef __DRV_CAMERA_H__
#define __DRV_CAMERA_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ter-util.h"
#include "ter-box.h"

typedef struct {
   glm::vec3 pos;
   glm::vec3 rot;
   float dist;
   bool dirty;
   TerBox box;
} TerCamera;

TerCamera * ter_camera_new(float px, float py, float pz,
                           float rx, float ry, float rz);
void ter_camera_free(TerCamera *cam);

glm::vec3 ter_camera_get_position(TerCamera *cam);
void ter_camera_set_position(TerCamera *cam, float px, float py, float pz);

glm::vec3 ter_camera_get_rotation (TerCamera *cam);
void ter_camera_set_rotation (TerCamera *cam, float rx, float ry, float rz);

void ter_camera_move(TerCamera *cam, float dx, float dy, float dz);
void ter_camera_rotate(TerCamera *cam, float rx, float ry, float rz);

glm::vec3 ter_camera_get_viewdir(TerCamera *cam);
void ter_camera_step(TerCamera *cam, float d, int stepX, int stepY, int stepZ);
void ter_camera_strafe(TerCamera *cam, float d);

void ter_camera_look_at(TerCamera *cam, float x, float y, float z);

glm::mat4 ter_camera_get_view_matrix(TerCamera *cam);
glm::mat4 ter_camera_get_rotation_matrix(TerCamera *cam);

void ter_camera_get_clipping_box_for_distance(TerCamera *cam, float dist,
                                              TerClipVolume *clip);

TerBox *ter_camera_get_box(TerCamera *cam);

#endif
