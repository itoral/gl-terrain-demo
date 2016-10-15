#include "main.h"

#include <glib.h>
#include <math.h>

#define OFFSET 15.0f

static void
calculate_dimensions(TerShadowBox *sb)
{
   float t = tanf(DEG_TO_RAD(TER_FOV));
   sb->far_width = TER_SHADOW_DISTANCE * t;
   sb->near_width = TER_NEAR_PLANE * t;
   sb->far_height = sb->far_width / TER_ASPECT_RATIO;
   sb->near_height = sb->near_width / TER_ASPECT_RATIO;
}

TerShadowBox *
ter_shadow_box_new(TerLight *light, TerCamera *camera)
{
   TerShadowBox *sb = (TerShadowBox *) g_new0(TerShadowBox, 1);
   sb->light_view_matrix = ter_light_get_view_matrix(light);
   sb->camera = camera;
   calculate_dimensions(sb);
   return sb;
}

void
ter_shadow_box_free(TerShadowBox *sb)
{
   g_free(sb);
}

float
ter_shadow_box_get_width(TerShadowBox *sb)
{
   return sb->maxX - sb->minX;
}

float
ter_shadow_box_get_height(TerShadowBox *sb)
{
   return sb->maxY - sb->minY;
}

float
ter_shadow_box_get_depth(TerShadowBox *sb)
{
   return sb->maxZ - sb->minZ;
}

glm::vec3
ter_shadow_box_get_center(TerShadowBox *sb)
{
   float x  = (sb->minX + sb->maxX) / 2.0f;
   float y  = (sb->minY + sb->maxY) / 2.0f;
   float z  = (sb->minZ + sb->maxZ) / 2.0f;
   glm::vec4 center(x, y, z, 1.0f);
   glm::mat4 inverse_light_view_matrix = glm::inverse(sb->light_view_matrix);
   glm::vec4 center_world_space = inverse_light_view_matrix * center;
   return vec3(center_world_space);
}

void
ter_shadow_box_get_clipping_box(TerShadowBox *sb,
                                glm::vec3 *c, float *w, float *h, float *d)
{
   glm::mat4 inverse_light_view_matrix = glm::inverse(sb->light_view_matrix);
   glm::vec4 v = inverse_light_view_matrix * vec4(sb->frustum[0], 1.0);

   float x0 = v.x;
   float x1 = v.x;
   float y0 = v.y;
   float y1 = v.y;
   float z0 = v.z;
   float z1 = v.z;
   for (int i = 1; i < 8; i++) {
      v = inverse_light_view_matrix * vec4(sb->frustum[i], 1.0);
      if (v.x < x0)
         x0 = v.x;
      else if (v.x > x1)
         x1 = v.x;

      if (v.y < y0)
         y0 = v.y;
      else if (v.y > y1)
         y1 = v.y;

      if (v.z < z0)
         z0 = v.z;
      else if (v.z > z1)
         z1 = v.z;
   }

   *c = ter_shadow_box_get_center(sb);
   *w = (x1 - x0) / 2.0f + OFFSET;
   *h = (y1 - y0) / 2.0f + OFFSET;
   *d = (z1 - z0) / 2.0f + OFFSET;
}

static glm::vec3
calculate_light_space_frustum_vertex(TerShadowBox *sb,
                                     glm::vec3 p, glm::vec3 dir, float width)
{
   ter_util_vec3_normalize(&dir);
   p = p + dir * width;
   glm::vec4 tmp = sb->light_view_matrix * glm::vec4(p.x, p.y, p.z, 1.0f);
   return vec3(tmp);
}

static void
calculate_frustum_vertices(TerShadowBox *sb,
                           glm::mat4 &rot_matrix,
                           glm::vec3 &forward_vector,
                           glm::vec3 &center_near,
                           glm::vec3 &center_far)
{
   glm::vec3 up_vector = vec3(rot_matrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
   glm::vec3 right_vector = glm::cross(forward_vector, up_vector);
   glm::vec3 down_vector = -up_vector;
   glm::vec3 left_vector = -right_vector;

   glm::vec3 far_top = center_far + up_vector * sb->far_height;
   glm::vec3 far_bottom = center_far + down_vector * sb->far_height;
   glm::vec3 near_top = center_near + up_vector * sb->near_height;
   glm::vec3 near_bottom = center_near + down_vector * sb->near_height;

   sb->frustum[0] =
      calculate_light_space_frustum_vertex(sb, far_top, right_vector, sb->far_width);
   sb->frustum[1] =
      calculate_light_space_frustum_vertex(sb, far_top, left_vector, sb->far_width);
   sb->frustum[2] =
      calculate_light_space_frustum_vertex(sb, far_bottom, right_vector, sb->far_width);
   sb->frustum[3] =
      calculate_light_space_frustum_vertex(sb, far_bottom, left_vector, sb->far_width);
   sb->frustum[4] =
      calculate_light_space_frustum_vertex(sb, near_top, right_vector, sb->near_width);
   sb->frustum[5] =
      calculate_light_space_frustum_vertex(sb, near_top, left_vector, sb->near_width);
   sb->frustum[6] =
      calculate_light_space_frustum_vertex(sb, near_bottom, right_vector, sb->near_width);
   sb->frustum[7] =
      calculate_light_space_frustum_vertex(sb, near_bottom, left_vector, sb->near_width);
}

static void
calculate_static_frustum_vertices(TerShadowBox *sb)
{
   sb->frustum[0] =
      vec3(sb->light_view_matrix * glm::vec4(TER_FAR_PLANE, TER_FAR_PLANE, -TER_FAR_PLANE, 1.0f));
   sb->frustum[1] =
      vec3(sb->light_view_matrix * glm::vec4(0.0f, TER_FAR_PLANE, -TER_FAR_PLANE, 1.0f));
   sb->frustum[2] =
      vec3(sb->light_view_matrix * glm::vec4(TER_FAR_PLANE, -30.0f, -TER_FAR_PLANE, 1.0f));
   sb->frustum[3] =
      vec3(sb->light_view_matrix * glm::vec4(0.0f, -30.0f, -TER_FAR_PLANE, 1.0f));

   sb->frustum[4] =
      vec3(sb->light_view_matrix * glm::vec4(TER_FAR_PLANE, TER_FAR_PLANE, 0.0f, 1.0f));
   sb->frustum[5] =
      vec3(sb->light_view_matrix * glm::vec4(0.0f, TER_FAR_PLANE, 0.0f, 1.0f));
   sb->frustum[6] =
      vec3(sb->light_view_matrix * glm::vec4(TER_FAR_PLANE, -TER_FAR_PLANE, 0.0f, 1.0f));
   sb->frustum[7] =
      vec3(sb->light_view_matrix * glm::vec4(0.0f, -TER_FAR_PLANE, 0.0f, 1.0f));
}

void
ter_shadow_box_update(TerShadowBox *sb)
{
   if (TER_DYNAMIC_LIGHT_ENABLE) {
      /* The GL camera looks at -Z */
      glm::mat4 rot_matrix = ter_camera_get_rotation_matrix(sb->camera);
      glm::vec3 forward_vector =
         vec3(rot_matrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

      glm::vec3 to_far = forward_vector * TER_SHADOW_DISTANCE;
      glm::vec3 to_near = forward_vector * TER_NEAR_PLANE;

      glm::vec3 center_near = sb->camera->pos + to_near;
      glm::vec3 center_far = sb->camera->pos + to_far;

      calculate_frustum_vertices(sb, rot_matrix, forward_vector,
                                 center_near, center_far);
   } else {
      calculate_static_frustum_vertices(sb);
   }

   sb->minX = sb->frustum[0].x;
   sb->maxX = sb->frustum[0].x;
   sb->minY = sb->frustum[0].y;
   sb->maxY = sb->frustum[0].y;
   sb->minZ = sb->frustum[0].z;
   sb->maxZ = sb->frustum[0].z;
   for (int i = 1; i < 8; i++) {
      if (sb->frustum[i].x > sb->maxX)
         sb->maxX = sb->frustum[i].x;
      else if (sb->frustum[i].x < sb->minX)
         sb->minX = sb->frustum[i].x;

      if (sb->frustum[i].y > sb->maxY)
         sb->maxY = sb->frustum[i].y;
      else if (sb->frustum[i].y < sb->minY)
         sb->minY = sb->frustum[i].y;

      if (sb->frustum[i].z > sb->maxZ)
         sb->maxZ = sb->frustum[i].z;
      else if (sb->frustum[i].z < sb->minZ)
         sb->minZ = sb->frustum[i].z;
   }

   sb->maxZ += OFFSET;
   sb->minZ -= OFFSET;

   sb->maxX += OFFSET;
   sb->minX -= OFFSET;

   sb->maxY += OFFSET;
   sb->minY -= OFFSET;
}
