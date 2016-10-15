#include "ter-shadow-renderer.h"
#include "main.h"

#include <glm/gtc/type_ptr.hpp>

static uint8_t instanced_buffer[TER_MODEL_MAX_INSTANCED_VBO_BYTES];

typedef struct {
   TerShadowRenderer *sr;
   TerShaderProgramShadowMap *sh, *sh_instanced;
   glm::vec3 clip_center;
   float clip_w, clip_h, clip_d;
   bool rendered;
   TerTerrain *terrain;
   TerObjectRenderer *obj_renderer;
   unsigned level;
} ShadowRendererRenderData;

TerShadowRenderer *
ter_shadow_renderer_new(TerLight *light, TerCamera *camera)
{
   TerShadowRenderer *sr = g_new0(TerShadowRenderer, 1);
   sr->light = light;
   sr->shadow_box = ter_shadow_box_new(light, camera);
   return sr;
}

void
ter_shadow_renderer_free(TerShadowRenderer *sr)
{
   ter_shadow_box_free(sr->shadow_box);
   g_free(sr);
}

static glm::mat4
compute_light_projection_matrix(TerShadowRenderer *sr, int level)
{
   float w = ter_shadow_box_get_width(sr->shadow_box, level);
   float h = ter_shadow_box_get_height(sr->shadow_box, level);
   float d = ter_shadow_box_get_depth(sr->shadow_box, level);

   glm::mat4 m(1.0f);
   m[0][0] =  2.0f / w;
   m[1][1] =  2.0f / h;
   m[2][2] = -2.0f / d;
   m[3][3] = 1.0f;

   return m;
}

static glm::mat4
compute_light_view_matrix(glm::vec3 dir, glm::vec3 pos)
{
   glm::mat4 view(1.0f);

   ter_util_vec3_normalize(&dir);

   float pitch = acosf(ter_util_vec3_module(dir, 1, 0, 1));
   view = glm::rotate(view, pitch, glm::vec3(1.0f, 0.0f, 0.0f));

   float yaw = atanf(dir.x / dir.z);
   yaw = dir.z > 0 ? yaw - PI : yaw;
   view = glm::rotate(view, -yaw, glm::vec3(0.0f, 1.0f, 0.0f));

   view = glm::translate(view, -pos);
   return view;
}

static void
render_start(TerShadowRenderer *sr, int level)
{
   glm::vec3 light_pos = vec3(ter_light_get_world_position(sr->light));
   glm::vec3 light_dir = -light_pos;

   sr->LightProjection[level] = compute_light_projection_matrix(sr, level);
   sr->LightView[level] = compute_light_view_matrix(light_dir,
      ter_shadow_box_get_center(sr->shadow_box, level));

   glEnable(GL_DEPTH_TEST);

   TerShadowMap *shadow_map =
      ter_shadow_box_get_shadow_map(sr->shadow_box, level);
   ter_render_texture_start(shadow_map->map);
   glClear(GL_DEPTH_BUFFER_BIT);
}

static void
render_stop(TerShadowRenderer *sr, int level)
{
   TerShadowMap *shadow_map =
      ter_shadow_box_get_shadow_map(sr->shadow_box, level);
   ter_render_texture_stop(shadow_map->map);
   glBindVertexArray(0);
   glDisableVertexAttribArray(0);
}

static inline bool
can_be_clipped(TerObject *o, glm::vec3 c, float w, float h, float d)
{
   /* It is important to use this method to get the position
    * because we want the position in the world and the position we store
    * in the object the result of correcting the center displacement and
    * applying the scaled height
    */
   glm::vec3 pos = ter_object_get_position(o);

   /* The shadow map uses orthographic projection and we really want to
    * render anything inside it, so the clipping is simpler than in the
    * case of the object renderer.
    */
   const float ow = ter_object_get_width(o);
   const float oh = ter_object_get_height(o);
   const float od = ter_object_get_depth(o);

   /* First we check if the object bounds are completely outside the
    * clipping cuboid. If that is the case the object is certainly outside
    * the viewing frustum and we can return early.
    */
   float x0 = pos.x - ow / 2.0f;
   float x1 = pos.x + ow / 2.0f;
   float y0 = pos.y;
   float y1 = pos.y + oh;
   float z0 = pos.z - od / 2.0f;
   float z1 = pos.z + od / 2.0f;

   return x1 < c.x - w || x0 > c.x + w ||
          z1 < c.z - d || z0 > c.z + d ||
          y1 < c.y - h || y0 > c.y + h;
}

static void
render_object_set(const char * key, GList *set, void *data)
{
   if (!set)
      return;

   GList *iter = set;
   ShadowRendererRenderData *d = (ShadowRendererRenderData *) data;

   TerObject *o = (TerObject *) iter->data;
   if (o->model->vao == 0) {
      d->rendered = false;
      return;
   }

   TerShadowRenderer *sr = d->sr;
   TerShaderProgramShadowMap *sh = d->sh_instanced;

   glUseProgram(sh->prog.program);

   ter_shader_program_shadow_map_load_VP(sh,
      &sr->LightProjection[d->level], &sr->LightView[d->level]);

   glm::vec3 cc = d->clip_center;
   unsigned num_clipped = 0;
   unsigned num_instances = 0;
   while (iter) {
      TerObject *o = (TerObject *) iter->data;
      if (!o->cast_shadow) {
         iter = g_list_next(iter);
         continue;
      }

      /* If rendering static lighting we render the full shadow map
       * just once, so we don't want to clip anything.
       */
      if (TER_SHADOW_RENDERER_ENABLE_CLIPPING &&
          TER_OBJECT_RENDERER_ENABLE_CLIPPING &&
          TER_DYNAMIC_LIGHT_ENABLE) {
         /* Don't render objects outside the shadow map clip volume */
         if (can_be_clipped(o, cc, d->clip_w, d->clip_h, d->clip_d)) {
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
   }

   ter_model_render_prepare_for_shadow_map(
      o->model, (float *) instanced_buffer, num_instances);

   glDrawArraysInstanced(GL_TRIANGLES, 0, o->model->vertices.size(),
                         num_instances);

   ter_model_render_finish_for_shadow_map(o->model);

   ter_dbg(LOG_RENDER, "\tSHADOW-RENDERER: INFO: clipped %u / %u objects\n",
           num_clipped, num_clipped + num_instances);
}

static void
render_terrain(TerTerrain *t, ShadowRendererRenderData *data)
{
   if (t->vao == 0) {
      data->rendered = false;
      return;
   }

   TerShaderProgramShadowMap *sh = data->sh;
   glUseProgram(sh->prog.program);

   glBindVertexArray(t->vao);
   glEnableVertexAttribArray(0);

   glm::mat4 Model = glm::mat4(1.0);
   ter_shader_program_shadow_map_load_MVP(
      sh,
      &data->sr->LightProjection[data->level],
      &data->sr->LightView[data->level],
      &Model);

   /* With static lighting we only render the shadow map once, so don't clip
    * anything: we want to render everything.
    */
   size_t buffer_offset = 0;
   if (TER_SHADOW_RENDERER_ENABLE_CLIPPING &&
       TER_TERRAIN_ENABLE_CLIPPING &&
       TER_DYNAMIC_LIGHT_ENABLE) {
      glm::vec3 clip_center;
      float clip_w, clip_h, clip_d;
      ter_shadow_box_get_clipping_box(data->sr->shadow_box, &clip_center,
                                      &clip_w, &clip_h, &clip_d,
                                      data->level);
      TerClipVolume clip;
      clip.x0 = clip_center.x - clip_w;
      clip.x1 = clip_center.x + clip_w;
      clip.z0 = clip_center.z - clip_d;
      clip.z1 = clip_center.z + clip_d;
      clip.y0 = clip_center.y - clip_h;
      clip.y1 = clip_center.y + clip_h;
      buffer_offset = ter_terrain_update_index_buffer_for_clip_volume(t, &clip);
   }

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, t->index_buf[t->ibuf_idx]);
   glDrawElements(GL_TRIANGLE_STRIP, t->num_indices,
                  GL_UNSIGNED_INT, (void *) buffer_offset);

   glBindVertexArray(0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

static void
render_shadow_map_level(TerShadowRenderer *sr,
                        ShadowRendererRenderData *data)
{
   int level = data->level;

   ter_shadow_box_get_clipping_box(sr->shadow_box, &data->clip_center,
                                   &data->clip_w, &data->clip_h, &data->clip_d,
                                   level);

   ter_dbg(LOG_RENDER,
           "SHADOW-RENDERER: INFO: level: %d, "
           "cuboid size: %.1f x %.1f x %.1f\n",
           level,
           ter_shadow_box_get_width(sr->shadow_box, level),
           ter_shadow_box_get_depth(sr->shadow_box, level),
           ter_shadow_box_get_height(sr->shadow_box, level));

   render_start(sr, level);

   /* Terrain shadows are very prone to shadow acne on the terrain surface.
    * To prevent that we have to increase the shadow acne factor in the
    * terrain shader (which offsets shadows casts by models, so it is not
    * great) or increase the resolution of the shadow map and/or its depth
    * and/or the PFC.
    */
   render_terrain(data->terrain, data);

   g_hash_table_foreach(data->obj_renderer->sets,
                        (GHFunc) render_object_set, data);

   render_stop(sr, level);
}

/*
 * Renders the scene objects (only the vertices) to a shadow map using the 
 * the shadow-map shader.
 */
bool
ter_shadow_renderer_render(TerShadowRenderer *sr)
{
   ter_shadow_box_update(sr->shadow_box);

   TerObjectRenderer *obj_renderer =
      (TerObjectRenderer *) ter_cache_get("rendering/obj-renderer");

   TerTerrain *terrain =
      (TerTerrain *) ter_cache_get("models/terrain");

   TerShaderProgramShadowMap *sh =
      (TerShaderProgramShadowMap *) ter_cache_get("program/shadow-map");

   TerShaderProgramShadowMap *sh_instanced =
      (TerShaderProgramShadowMap *) ter_cache_get("program/shadow-map-instanced");

   ShadowRendererRenderData data;
   data.sr = sr;
   data.sh = sh;
   data.sh_instanced = sh_instanced;
   data.rendered = true;
   data.terrain = terrain;
   data.obj_renderer = obj_renderer;

   for (unsigned level = 0; level < sr->shadow_box->csm_levels; level++) {
      data.level = level;
      render_shadow_map_level(sr, &data);
   }

   return data.rendered;
}

glm::mat4
ter_shadow_renderer_get_shadow_map_space_vp(TerShadowRenderer *sr,
                                            unsigned level)
{
   glm::mat4 offset(1.0f);
   offset = glm::translate(offset, glm::vec3(0.5f, 0.5f, 0.5f));
   offset = glm::scale(offset, glm::vec3(0.5f, 0.5f, 0.5f));
   return offset * sr->LightProjection[level] * sr->LightView[level];
}

