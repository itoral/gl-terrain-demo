#include "ter-camera.h"
#include "ter-util.h"
#include "ter-cache.h"
#include <glib.h>

TerCamera *
ter_camera_new(float px, float py, float pz,
               float rx, float ry, float rz)
{
   TerCamera *cam = g_new0(TerCamera, 1);
   ter_camera_set_position(cam, px, py, pz);
   ter_camera_set_rotation(cam, rx, ry, rz);

   cam->box.center = cam->pos;
   cam->box.w = cam->box.h = cam->box.d = 2.0f * TER_NEAR_PLANE;

   return cam;
}

void
ter_camera_free(TerCamera *cam)
{
   g_free(cam);
}

glm::vec3
ter_camera_get_position(TerCamera *cam)
{
   return cam->pos;
}

void
ter_camera_set_position(TerCamera *cam, float px, float py, float pz)
{
   cam->pos.x = px;
   cam->pos.y = py;
   cam->pos.z = pz;
   cam->dirty = true;
}

glm::vec3
ter_camera_get_rotation(TerCamera *cam)
{
   return cam->rot;
}

void
ter_camera_set_rotation(TerCamera *cam, float rx, float ry, float rz)
{
   cam->rot.x = rx;
   cam->rot.y = ry;
   cam->rot.z = rz;
   cam->dirty = true;
}

void
ter_camera_move(TerCamera *cam, float dx, float dy, float dz)
{
   cam->pos.x += dx;
   cam->pos.y += dy;
   cam->pos.z += dz;
   cam->dirty = true;
}

void
ter_camera_rotate(TerCamera *cam, float rx, float ry, float rz)
{
   cam->rot.x += rx;
   cam->rot.y += ry;
   cam->rot.z += rz;

   if (cam->rot.x >= 360.0f)
      cam->rot.x -= 360.0f;
   else if (cam->rot.x <= -360.0f)
      cam->rot.x += 360.0f;

   if (cam->rot.y > 360.0f)
      cam->rot.y -= 360.0f;
   else if (cam->rot.y <= -360.0f)
      cam->rot.y += 360.0f;

   if (cam->rot.z > 360.0f)
      cam->rot.z -= 360.0f;
   else if (cam->rot.z <= -360.0f)
      cam->rot.z += 360.0f;

   cam->dirty = true;
}

/**
 * Obtains a normalized vector describing the camera viewing direction
 */
glm::vec3
ter_camera_get_viewdir(TerCamera *cam)
{
   glm::vec3 v1, v2;

   /* Rotate around Y-axis */
   float angle = DEG_TO_RAD(cam->rot.y + 90.0);
   v1.x =  cos(angle);
   v1.z = -sin(angle);

   /* Rotate around X-axis */
   angle = DEG_TO_RAD(cam->rot.x);
	float cosX = cos(angle);
	v2.x = v1.x * cosX;
	v2.z = v1.z * cosX;
	v2.y = sin(angle);

   /* Rotate around Z-axis (not supportted!) */

   return v2;
}

/**
 * Move camera along the camera viewing direction
 * StepX, StepY, StepZ enable (1) or disable (0) movement along specific axis.
 */
void
ter_camera_step(TerCamera *cam, float d, int stepX, int stepY, int stepZ)
{
   glm::vec3 view;
   view = ter_camera_get_viewdir(cam);
   if (!stepX)
      view.x = 0.0f;
   if (!stepY)
      view.y = 0.0f;
   if (!stepZ)
      view.z = 0.0f;
   cam->pos.x += view.x * d;
   cam->pos.y += view.y * d;
   cam->pos.z += view.z * d;
   cam->dirty = true;
}

/**
 * Strafe camera (only X-Z plane supported)
 */
void
ter_camera_strafe(TerCamera *cam, float d)
{
   glm::vec3 view, strafe;
   view = ter_camera_get_viewdir(cam);
   strafe.x = view.z;
   strafe.y = 0.0f;
   strafe.z = -view.x;
   cam->pos.x += strafe.x * d;
   cam->pos.y += strafe.y * d;
   cam->pos.z += strafe.z * d;
   cam->dirty = true;
}


/**
 * Sets the camera to look at a specific point in space
 */
void
ter_camera_look_at(TerCamera *cam, float x, float y, float z)
{
   glm::vec3 target;
   float dist;
   float cosAngle, sinAngle, angle;

   target.x = x - cam->pos.x;
   target.y = y - cam->pos.y;
   target.z = z - cam->pos.z;

   /* Compute rotation on Y-asis */
   dist = ter_util_vec3_module(target, 1, 0, 1);
   cosAngle = target.x / dist;
   angle = acos(cosAngle);
   angle = RAD_TO_DEG(angle) - 90.0f;
   if (target.z > 0.0f)
     angle += (90.0f - angle) * 2.0f;
   cam->rot.y = angle;

   /* Compute rotation on X-axis */
   dist = ter_util_vec3_module(target, 1, 1, 1);
   sinAngle = target.y / dist;
   angle = asin(sinAngle);
   angle = RAD_TO_DEG(angle);
   cam->rot.x = angle;

   cam->rot.z = 0.0f;
   cam->dirty = true;
}

glm::mat4
ter_camera_get_rotation_matrix(TerCamera *cam)
{
   glm::mat4 mat(1.0);
   float rx = DEG_TO_RAD(cam->rot.x);
   float ry = DEG_TO_RAD(cam->rot.y);
   float rz = DEG_TO_RAD(cam->rot.z);
   mat = glm::rotate(mat, rz, glm::vec3(0, 0, 1));
   mat = glm::rotate(mat, ry, glm::vec3(0, 1, 0));
   mat = glm::rotate(mat, rx, glm::vec3(1, 0, 0));
   return mat;
}

glm::mat4
ter_camera_get_view_matrix(TerCamera *cam)
{
   glm::mat4 mat(1.0);
   float rx = DEG_TO_RAD(cam->rot.x);
   float ry = DEG_TO_RAD(cam->rot.y);
   float rz = DEG_TO_RAD(cam->rot.z);
   mat = glm::rotate(mat, -rx, glm::vec3(1, 0, 0));
   mat = glm::rotate(mat, -ry, glm::vec3(0, 1, 0));
   mat = glm::rotate(mat, -rz, glm::vec3(0, 0, 1));
   mat = glm::translate(mat, -cam->pos);
   return mat;
}

static void
compute_frustum_vertices_for_dist(TerCamera *cam, float dist, glm::vec3 *f)
{
   /* The GL camera looks at -Z */
   glm::mat4 rot_matrix = ter_camera_get_rotation_matrix(cam);
   glm::vec3 forward_vector =
      vec3(rot_matrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

   glm::vec3 to_far = forward_vector * dist;
   glm::vec3 to_near = forward_vector * TER_NEAR_PLANE;
   glm::vec3 center_far = cam->pos + to_far;
   glm::vec3 center_near = cam->pos + to_near;

   glm::vec3 up_vector = vec3(rot_matrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
   glm::vec3 right_vector = glm::cross(forward_vector, up_vector);
   ter_util_vec3_normalize(&up_vector);
   ter_util_vec3_normalize(&right_vector);

   float t = tanf(DEG_TO_RAD(TER_FOV));
   float far_width = dist * t;
   float far_height = far_width / TER_ASPECT_RATIO;
   float near_width = TER_NEAR_PLANE * t;
   float near_height = near_width / TER_ASPECT_RATIO;

   glm::vec3 far_top = center_far + up_vector * far_height;
   glm::vec3 far_bottom = center_far + up_vector * (-far_height);
   glm::vec3 near_top = center_near + up_vector * near_height;
   glm::vec3 near_bottom = center_near + up_vector * (-near_height);

   f[0] = far_top + right_vector * far_width;
   f[1] = far_top + right_vector * (-far_width);
   f[2] = far_bottom + right_vector * far_width;
   f[3] = far_bottom + right_vector * (-far_width);

   f[4] = near_top + right_vector * near_width;
   f[5] = near_top + right_vector * (-near_width);
   f[6] = near_bottom + right_vector * near_width;
   f[7] = near_bottom + right_vector * (-near_width);
}

void
ter_camera_get_clipping_box_for_distance(TerCamera *cam, float dist,
                                         TerClipVolume *clip)
{
   glm::vec3 f[8];
   compute_frustum_vertices_for_dist(cam, dist, f);

   glm::mat4 inverse_view_matrix =
      glm::inverse(ter_camera_get_view_matrix(cam));

   clip->x0 = f[0].x;
   clip->x1 = f[0].x;
   clip->y0 = f[0].y;
   clip->y1 = f[0].y;
   clip->z0 = f[0].z;
   clip->z1 = f[0].z;
   for (int i = 1; i < 8; i++) {
      glm::vec3 v = f[i];
      if (v.x < clip->x0)
         clip->x0 = v.x;
      else if (v.x > clip->x1)
         clip->x1 = v.x;

      if (v.y < clip->y0)
         clip->y0 = v.y;
      else if (v.y > clip->y1)
         clip->y1 = v.y;

      if (v.z < clip->z0)
         clip->z0 = v.z;
      else if (v.z > clip->z1)
         clip->z1 = v.z;
   }
}

TerBox *
ter_camera_get_box(TerCamera *cam)
{
   cam->box.center = cam->pos;
   return &cam->box;
}
