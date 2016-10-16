#include "main.h"

#include <glib.h>
#include <math.h>

/* This offset is added to the shadow frustum in order to make sure
 * that tall shadow casters are kept in the shadow volume and they
 * are not clipped away while they are still casting visible shadows
 */
#define OFFSET 15.0f

/*
 * Computes the sizes of each CSM level */
static void
calculate_dimensions(TerShadowBox *sb)
{
   float t = tanf(DEG_TO_RAD(TER_FOV));

   /* The asserts below implement a few sanity checks to ensure that the CSM
    * has been configured properly:
    * 1. Maximum number of CSM values is 4.
    * 2. Last level distance must be 1
    * 3. All distance values in (0, 1]
    * 4. All distance values strictly increasing.
    */
   assert(TER_SHADOW_CSM_NUM_LEVELS >= 1 &&
          TER_SHADOW_CSM_NUM_LEVELS <= 4);
   sb->csm_levels = TER_SHADOW_CSM_NUM_LEVELS;

   assert(TER_SHADOW_CSM_DISTANCES[sb->csm_levels - 1] == 1.0f);

   for (unsigned i = 0; i < sb->csm_levels; i++) {
      assert(TER_SHADOW_CSM_DISTANCES[i] > 0.0f &&
             TER_SHADOW_CSM_DISTANCES[i] <= 1.0f);
      assert(i == 0 ||
             TER_SHADOW_CSM_DISTANCES[i] > TER_SHADOW_CSM_DISTANCES[i - 1]);

      sb->csm[i].far_dist = TER_SHADOW_DISTANCE * TER_SHADOW_CSM_DISTANCES[i];
      sb->csm[i].near_dist =
         (i == 0) ? TER_NEAR_PLANE : sb->csm[i - 1].far_dist;
      sb->csm[i].far_width = sb->csm[i].far_dist * t;
      sb->csm[i].near_width = sb->csm[i].near_dist * t;
      sb->csm[i].far_height = sb->csm[i].far_width / TER_ASPECT_RATIO;
      sb->csm[i].near_height = sb->csm[i].near_width / TER_ASPECT_RATIO;
   }
}

/* Allocates the shadow map texture for each level
 *
 * Shadow maps are high-res depth textures and rendering to them is expensive
 * (specially the inital clear, at least on Intel). By lowering the size
 * of some of the levels we can gain some performance at the expense
 * of losing some quality (at the distances covered by these levels).
 */
static void
create_shadow_maps(TerShadowBox *sb)
{
   for (unsigned i = 0; i < sb->csm_levels; i++) {
      float size = TER_SHADOW_MAP_SIZE * TER_SHADOW_CSM_MAP_SIZES[i];
      assert(size > 0.0f);
      sb->csm[i].shadow_map = ter_shadow_map_new(size, size);
   }
}

TerShadowBox *
ter_shadow_box_new(TerLight *light, TerCamera *camera)
{
   TerShadowBox *sb = (TerShadowBox *) g_new0(TerShadowBox, 1);
   sb->light_view_matrix = ter_light_get_view_matrix(light);
   sb->camera = camera;
   calculate_dimensions(sb);
   create_shadow_maps(sb);
   return sb;
}

void
ter_shadow_box_free(TerShadowBox *sb)
{
   for (unsigned i = 0; i < sb->csm_levels; i++)
      ter_shadow_map_free(sb->csm[i].shadow_map);
   g_free(sb);
}

float
ter_shadow_box_get_width(TerShadowBox *sb, int l)
{
   return sb->csm[l].maxX - sb->csm[l].minX;
}

float
ter_shadow_box_get_height(TerShadowBox *sb, int l)
{
   return sb->csm[l].maxY - sb->csm[l].minY;
}

float
ter_shadow_box_get_depth(TerShadowBox *sb, int l)
{
   return sb->csm[l].maxZ - sb->csm[l].minZ;
}

/* Gets the center (in world cooridnates) of the shadow volume for a particular
 * CSM level. We use this to compute the clipping volume for the level and also
 * to position the volume and also to compute the ortographic projection of the
 * volume when we render the shadow map.
 */
glm::vec3
ter_shadow_box_get_center(TerShadowBox *sb, int l)
{
   TerShadowBoxLevel *lvl = &sb->csm[l];

   float x  = (lvl->minX + lvl->maxX) / 2.0f;
   float y  = (lvl->minY + lvl->maxY) / 2.0f;
   float z  = (lvl->minZ + lvl->maxZ) / 2.0f;
   glm::vec4 center(x, y, z, 1.0f);
   glm::mat4 inverse_light_view_matrix = glm::inverse(sb->light_view_matrix);
   glm::vec4 center_world_space = inverse_light_view_matrix * center;
   return vec3(center_world_space);
}

/* Computes the an axis-aligned clipping volume for a particular CSM
 * level in world coordinates.
 */
void
ter_shadow_box_get_clipping_box(TerShadowBox *sb,
                                glm::vec3 *c, float *w, float *h, float *d,
                                int l)
{
   TerShadowBoxLevel *lvl = &sb->csm[l];

   glm::mat4 inverse_light_view_matrix = glm::inverse(sb->light_view_matrix);
   glm::vec4 v = inverse_light_view_matrix * vec4(lvl->frustum[0], 1.0);

   float x0 = v.x;
   float x1 = v.x;
   float y0 = v.y;
   float y1 = v.y;
   float z0 = v.z;
   float z1 = v.z;

   for (int i = 1; i < 8; i++) {
      v = inverse_light_view_matrix * vec4(lvl->frustum[i], 1.0);
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

   *c = ter_shadow_box_get_center(sb, l);
   *w = (x1 - x0) / 2.0f;
   *h = (y1 - y0) / 2.0f;
   *d = (z1 - z0) / 2.0f;

   *w += OFFSET;
   *h += OFFSET;
   *d += OFFSET;
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
                           TerShadowBoxLevel *l,
                           glm::mat4 &rot_matrix,
                           glm::vec3 &forward_vector,
                           glm::vec3 &center_near,
                           glm::vec3 &center_far)
{
   glm::vec3 up_vector = vec3(rot_matrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
   glm::vec3 right_vector = glm::cross(forward_vector, up_vector);
   glm::vec3 down_vector = -up_vector;
   glm::vec3 left_vector = -right_vector;

   glm::vec3 far_top = center_far + up_vector * l->far_height;
   glm::vec3 far_bottom = center_far + down_vector * l->far_height;
   glm::vec3 near_top = center_near + up_vector * l->near_height;
   glm::vec3 near_bottom = center_near + down_vector * l->near_height;

   l->frustum[0] =
      calculate_light_space_frustum_vertex(sb, far_top,
                                           right_vector, l->far_width);
   l->frustum[1] =
      calculate_light_space_frustum_vertex(sb, far_top,
                                           left_vector, l->far_width);
   l->frustum[2] =
      calculate_light_space_frustum_vertex(sb, far_bottom,
                                           right_vector, l->far_width);
   l->frustum[3] =
      calculate_light_space_frustum_vertex(sb, far_bottom,
                                           left_vector, l->far_width);
   l->frustum[4] =
      calculate_light_space_frustum_vertex(sb, near_top,
                                           right_vector, l->near_width);
   l->frustum[5] =
      calculate_light_space_frustum_vertex(sb, near_top,
                                           left_vector, l->near_width);
   l->frustum[6] =
      calculate_light_space_frustum_vertex(sb, near_bottom,
                                           right_vector, l->near_width);
   l->frustum[7] =
      calculate_light_space_frustum_vertex(sb, near_bottom,
                                           left_vector, l->near_width);
}

static void
shadow_box_update_dim(TerShadowBox *sb, TerShadowBoxLevel *l,
                      glm::mat4 &rot_matrix, glm::vec3 &forward_vector)
{
   glm::vec3 to_far = forward_vector * l->far_dist;
   glm::vec3 to_near = forward_vector * l->near_dist;

   glm::vec3 center_near = sb->camera->pos + to_near;
   glm::vec3 center_far = sb->camera->pos + to_far;

   calculate_frustum_vertices(sb, l, rot_matrix, forward_vector,
                              center_near, center_far);
}

static void
update_dimensions_from_frustum(TerShadowBoxLevel *l)
{
   l->minX = l->frustum[0].x;
   l->maxX = l->frustum[0].x;
   l->minY = l->frustum[0].y;
   l->maxY = l->frustum[0].y;
   l->minZ = l->frustum[0].z;
   l->maxZ = l->frustum[0].z;

   for (int i = 1; i < 8; i++) {
      if (l->frustum[i].x > l->maxX)
         l->maxX = l->frustum[i].x;
      else if (l->frustum[i].x < l->minX)
         l->minX = l->frustum[i].x;

      if (l->frustum[i].y > l->maxY)
         l->maxY = l->frustum[i].y;
      else if (l->frustum[i].y < l->minY)
         l->minY = l->frustum[i].y;

      if (l->frustum[i].z > l->maxZ)
         l->maxZ = l->frustum[i].z;
      else if (l->frustum[i].z < l->minZ)
         l->minZ = l->frustum[i].z;
   }

   l->maxZ += OFFSET;
   l->minZ -= OFFSET;

   l->maxX += OFFSET;
   l->minX -= OFFSET;

   l->maxY += OFFSET;
   l->minY -= OFFSET;
}

/* Re-computes the size and orientation of the shadow volume for all
 * CSM levels.
 */
void
ter_shadow_box_update(TerShadowBox *sb)
{
   /* The GL camera looks at -Z */
   glm::mat4 rot_matrix = ter_camera_get_rotation_matrix(sb->camera);
   glm::vec3 forward_vector =
      vec3(rot_matrix * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));

   for (unsigned i = 0; i < sb->csm_levels; i++) {
      shadow_box_update_dim(sb, &sb->csm[i], rot_matrix, forward_vector);
      update_dimensions_from_frustum(&sb->csm[i]);
   }
}

TerShadowMap *
ter_shadow_box_get_shadow_map(TerShadowBox *sb, int l)
{
   return sb->csm[l].shadow_map;
}

float
ter_shadow_box_get_far_distance(TerShadowBox *sb, int l)
{
   return sb->csm[l].far_dist;
}

float
ter_shadow_box_get_map_size(TerShadowBox *sb, int l)
{
   return sb->csm[l].shadow_map->map->width;
}
