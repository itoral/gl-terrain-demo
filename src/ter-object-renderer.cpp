#include "ter-object-renderer.h"

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#include <glm/gtc/type_ptr.hpp>

#include "ter-camera.h"
#include "ter-cache.h"

static uint8_t instanced_buffer[TER_MODEL_MAX_INSTANCED_VBO_BYTES];

/* The object renderer keeps a map of all instances of a particular model
 * in the scene. When we need to render all objects, it renderes instances
 * sorted by model, and binds state for each model only once.
 */

typedef struct {
   TerClipVolume *clip;
   float clip_far_plane;
   float render_far_plane;
   bool enable_shadows;
   unsigned shadow_pfc;
   const char *stage;
} TerObjectRendererData;

TerObjectRenderer *
ter_object_renderer_new()
{
   TerObjectRenderer *r = (TerObjectRenderer *) g_new0(TerObjectRenderer, 1);
   r->sets = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
   return r;
}

void
ter_object_renderer_free(TerObjectRenderer *r)
{
   g_list_free_full(r->all, (GDestroyNotify) ter_object_free);
   g_list_free(r->solid);
   g_hash_table_destroy(r->sets);
   g_free(r);
}

void
ter_object_renderer_add_object(TerObjectRenderer *r, TerObject *o)
{
   const char *key = o->model->name;
   GList *obj_list = (GList *) g_hash_table_lookup(r->sets, key);
   obj_list = g_list_prepend(obj_list, o);
   g_hash_table_insert(r->sets, g_strdup(key), obj_list);
   r->all = g_list_prepend(r->all, o);
   if (o->can_collide)
      r->solid = g_list_prepend(r->solid, o);
}

GList *
ter_object_renderer_get_all(TerObjectRenderer *r)
{
   return r->all;
}

GList *
ter_object_renderer_get_solid(TerObjectRenderer *r)
{
   return r->solid;
}

static inline bool
can_be_clipped(TerObject *o, TerClipVolume *clip, float far_plane)
{
   /* It is important to use this method to get the position
    * because we want the position in the world and the position we store
    * in the object the result of correcting the center displacement and
    * applying the scaled height
    */
   glm::vec3 pos = ter_object_get_position(o);

   const float w = ter_object_get_width(o);
   const float h = ter_object_get_height(o);
   const float d = ter_object_get_depth(o);

   /* First we check if the object bounds are completely outside the
    * clipping cuboid. If that is the case the object is certainly outside
    * the viewing frustum and we can return early.
    */
   float x0 = pos.x - w / 2.0f;
   float x1 = pos.x + w / 2.0f;
   float y0 = pos.y;
   float y1 = pos.y + h;
   float z0 = pos.z - d / 2.0f;
   float z1 = pos.z + d / 2.0f;

   bool outside = x1 < clip->x0 || x0 > clip->x1 ||
                  z1 < clip->z0 || z0 > clip->z1 ||
                  y1 < clip->y0 || y0 > clip->y1;
   if (outside)
      return true;

   /* Otherwise, we check the angle of the vector from the camera to the each
    * of the object bounds and the camera view vector and match it against the
    * FOV and the far plane to see if the object is within the perspective
    * frustum.
    */
   glm::vec3 bounds[8];
   bounds[0] = glm::vec3(x0, y0, z0);
   bounds[1] = glm::vec3(x0, y0, z1);
   bounds[2] = glm::vec3(x0, y1, z0);
   bounds[3] = glm::vec3(x0, y1, z1);
   bounds[4] = glm::vec3(x1, y0, z0);
   bounds[5] = glm::vec3(x1, y0, z1);
   bounds[6] = glm::vec3(x1, y1, z0);
   bounds[7] = glm::vec3(x1, y1, z1);

   TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");
   glm::vec3 view_dir = ter_camera_get_viewdir(cam);
   ter_util_vec3_normalize(&view_dir);
   for (int i = 0; i < 8; i++) {
      /* Check if the boundary is too far away (or too close). If it is, then
       * the boundary is outside the frustum, check the remaining boundaries.
       */
      glm::vec3 dir_from_cam = bounds[i] - cam->pos;
      float dist = ter_util_vec3_module(dir_from_cam, 1, 1, 1);
      if (dist > far_plane || dist < TER_NEAR_PLANE)
         continue;

      /* Check if the angle is inside the FOV, if it is then at least this
       * boundary is visible and we can't clip the object.
       *
       * FIXME: I think we need a way to factor in the aspect ratio here
       *        right now the Y-clip region seems too large.
       */
      ter_util_vec3_normalize(&dir_from_cam);
      float dot = ter_util_vec3_dot(dir_from_cam, view_dir);
      /* The abs. value of the result must be in [-1, 1] but there can be
       * rounding errors that can lead to a values outside and that can confuse
       * acos(), so clamp.
       */
      dot = CLAMP(dot, -1.0f, 1.0f);
      float angle = acos(dot);
      if (fabsf(angle) <= DEG_TO_RAD(TER_FOV + 5.0f))
         return false;     
   }

   /* None of the object boundaries is inside the perspective frustum, we can
    * clip the object
    */
   return true;
}

static void
render_object_set_clipped(const char *key, GList *set, void *data)
{
   if (!set)
      return;

   TerObjectRendererData *d = (TerObjectRendererData *) data;

   unsigned num_clipped = 0;
   unsigned num_instances = 0;
   GList *iter = set;
   while (iter) {
      TerObject *o = (TerObject *) iter->data;

      if (TER_OBJECT_RENDERER_ENABLE_CLIPPING) {
         /* Skip objects outside the viewing frustum */
         if (can_be_clipped(o, d->clip, d->clip_far_plane)) {
            iter = g_list_next(iter);
            num_clipped++;
            continue;
         }
      }

      glm::mat4 Model = ter_object_get_model_matrix(o);
      float *Model_fptr = glm::value_ptr(Model);

      unsigned offset = num_instances * TER_MODEL_INSTANCED_ITEM_SIZE;
      unsigned model_size = 16 * sizeof(float);
      memcpy(instanced_buffer + offset, Model_fptr, model_size);
      offset += model_size;

      unsigned variant_idx_size = sizeof(int);
      unsigned variant_idx = o->variant * TER_MODEL_MAX_MATERIALS;
      memcpy(instanced_buffer + offset, &variant_idx, variant_idx_size);
      offset += variant_idx_size;

      iter = g_list_next(iter);
      num_instances++;
   };

   TerObject *o = (TerObject *) set->data;
   ter_model_render_prepare(o->model, (float *) instanced_buffer, num_instances,
                            d->clip_far_plane, d->render_far_plane,
                            d->enable_shadows, d->shadow_pfc);

   glDrawArraysInstanced(GL_TRIANGLES, 0, o->model->vertices.size(),
                         num_instances);

   ter_model_render_finish(o->model);

   ter_dbg(LOG_RENDER, "\tOBJ-RENDERER: INFO: clipped %u / %u objects\n",
           num_clipped, num_clipped + num_instances);
}

/* Renders the objects clipped to the provided clip cuboid first and to a
 * viewing frustum with clip_far_plane second. The render far plane is used
 * to create a projection matrix for rendering. Usually, the clip far plane
 * and the render far plane will be the same, but in cases like the refraction
 * texture that need to render depth information with the default far plane
 * but fade objects against the clip plane, they can be different.
 */
void
ter_object_renderer_render_clipped(TerObjectRenderer *r,
                                   float clip_far_plane,
                                   float render_far_plane,
                                   bool enable_blending,
                                   bool enable_shadows,
                                   unsigned shadow_pfc,
                                   TerClipVolume *clip,
                                   const char *stage)
{
   ter_dbg(LOG_RENDER,
           "OBJ-RENDERER: INFO: stage: %s: blending: %s, shadows: %s\n", stage,
           enable_blending ? "on" : "off", enable_shadows ? "on" : "off");

   if (enable_blending)
      glEnable(GL_BLEND);

   TerObjectRendererData data;
   data.clip = clip;
   data.clip_far_plane = clip_far_plane;
   data.render_far_plane = render_far_plane;
   data.enable_shadows = enable_shadows;
   data.shadow_pfc = shadow_pfc;
   data.stage = stage;
   g_hash_table_foreach(r->sets, (GHFunc) render_object_set_clipped, &data);

   if (enable_blending)
      glDisable(GL_BLEND);
}

static void
load_object_box_data(TerObject *o, glm::vec3 *vdata)
{
   TerBox *box = ter_object_get_box(o);
   glm::vec3 c = box->center;
   float w = box->w;
   float h = box->h;
   float d = box->d;

   /* Bottom */
   vdata[0] = glm::vec3(c.x - w, c.y - h, c.z - d);
   vdata[1] = glm::vec3(c.x + w, c.y - h, c.z - d);
   vdata[2] = glm::vec3(c.x + w, c.y - h, c.z - d);
   vdata[3] = glm::vec3(c.x + w, c.y - h, c.z + d);
   vdata[4] = glm::vec3(c.x + w, c.y - h, c.z + d);
   vdata[5] = glm::vec3(c.x - w, c.y - h, c.z + d);
   vdata[6] = glm::vec3(c.x - w, c.y - h, c.z + d);
   vdata[7] = glm::vec3(c.x - w, c.y - h, c.z - d);

   /* Top */
   vdata[8] = glm::vec3(c.x - w, c.y + h, c.z - d);
   vdata[9] = glm::vec3(c.x + w, c.y + h, c.z - d);
   vdata[10] = glm::vec3(c.x + w, c.y + h, c.z - d);
   vdata[11] = glm::vec3(c.x + w, c.y + h, c.z + d);
   vdata[12] = glm::vec3(c.x + w, c.y + h, c.z + d);
   vdata[13] = glm::vec3(c.x - w, c.y + h, c.z + d);
   vdata[14] = glm::vec3(c.x - w, c.y + h, c.z + d);
   vdata[15] = glm::vec3(c.x - w, c.y + h, c.z - d);

   /* Sides */
   vdata[16] = glm::vec3(c.x - w, c.y - h, c.z - d);
   vdata[17] = glm::vec3(c.x - w, c.y + h, c.z - d);
   vdata[18] = glm::vec3(c.x + w, c.y - h, c.z - d);
   vdata[19] = glm::vec3(c.x + w, c.y + h, c.z - d);
   vdata[20] = glm::vec3(c.x + w, c.y - h, c.z + d);
   vdata[21] = glm::vec3(c.x + w, c.y + h, c.z + d);
   vdata[22] = glm::vec3(c.x - w, c.y - h, c.z + d);
   vdata[23] = glm::vec3(c.x - w, c.y + h, c.z + d);
}

static void
render_object_set_boxes(const char *key, GList *set, void *data)
{
   /* We should really not allocate GL resources in local static variables
    * But this is a debug mode, so we don't care too much
    */
   static unsigned vertex_buf = 0;
   static unsigned vao = 0;
   static glm::vec3 vdata[24];

   if (!set)
      return;

   if (vao == 0) {
      glGenBuffers(1, &vertex_buf);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vdata), NULL,
                   GL_DYNAMIC_DRAW);

      glGenVertexArrays(1, &vao);
      glBindVertexArray(vao);

      glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(
         0,                  // Attribute index
         3,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );
   } else {
      glBindVertexArray(vao);
      glEnableVertexAttribArray(0);
   }

   TerShaderProgramBox *sh =
      (TerShaderProgramBox *) ter_cache_get("program/box");
   glUseProgram(sh->prog.program);

   glm::mat4 *Projection = (glm::mat4 *) ter_cache_get("matrix/Projection");
   glm::mat4 *View = (glm::mat4 *) ter_cache_get("matrix/View");
   glm::mat4 VP = (*Projection) * (*View);


   /* We use Axis Aligned Bounding Boxes and each time we retrieve the box
    * for an object we re-calculate applying transforms it if necessary
    * so we don't need to set a Model for any box.
    */
   ter_shader_program_box_load_MVP(sh, &VP);

   /* Not really caring about performance here, but it is okay,
    * it is a debug feature
    */
   GList *iter = set;
   while (iter) {
      TerObject *o = (TerObject *) iter->data;

      /* Don't render bounding boxes for objects that don't produce collisions
       */
      if (!o->can_collide) {
         iter = g_list_next(iter);
         continue;
      }

      load_object_box_data(o, vdata);
      glBindBuffer(GL_ARRAY_BUFFER, vertex_buf);
      glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vdata), vdata);

      glDrawArrays(GL_LINES, 0, 24);

      iter = g_list_next(iter);
   }

   glBindVertexArray(0);
}

void
ter_object_renderer_render_boxes(TerObjectRenderer *r)
{
   g_hash_table_foreach(r->sets, (GHFunc) render_object_set_boxes, NULL);
}
