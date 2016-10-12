#include "main.h"
#include "ter-water-tile.h"
#include "ter-shader-program.h"

#include <glib.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#include "main-constants.h"

static void
set_vertices(TerWaterTile *t)
{
   float width = fabsf(t->x1 - t->x0);
   assert(fmodf(width, t->tile_size) == 0.0f);

   float depth = fabsf(t->z1 - t->z0);
   assert(fmodf(depth, t->tile_size) == 0.0f);

   unsigned num_cols = width / t->tile_size;
   assert(num_cols >= 2);

   unsigned num_rows = depth / t->tile_size;
   assert(num_rows >= 2);


   /* Use triangle strips to build the mesh for better performance and lower
    * memory usage.
    */
   unsigned num_vertices =
      (num_cols - 1) * (num_rows * 2) + (num_cols - 2) + (num_cols - 1);
   t->vertices = g_new0(glm::vec3, num_vertices);

   t->num_vertices = 0;
   for (int c = 0; c < num_cols - 1; c++) {
      for (int r = 0; r < num_rows; r++) {
         float x = t->x0 + c * t->tile_size;
         float z = t->z0 - r * t->tile_size;
         float y = t->h;

         glm::vec3 v0 = glm::vec3(x, y, z);

         /* Strip start. Link up to the next strip using degenerate triangles */
         if (c > 0 && r == 0) {
            t->vertices[t->num_vertices++] = glm::vec3(x, y, z);
         }
         t->vertices[t->num_vertices++] = glm::vec3(x, y, z);

         x += t->tile_size;
         t->vertices[t->num_vertices++] = glm::vec3(x, y, z);

         /* Strip end. Link up to the next strip using degenerate triangles */
         if (r == num_rows - 1 && c < num_cols - 1) {
            t->vertices[t->num_vertices++] = glm::vec3(x, y, z);
         }
      }
   }

   assert(t->num_vertices == num_vertices);
}

TerWaterTile *
ter_water_tile_new(float x0, float z0, float x1, float z1, float h,
                   float tile_size, unsigned dudv_tex, unsigned normal_tex)
{
   TerWaterTile *t = g_new0(TerWaterTile, 1);

   assert(x0 < x1 && z1 < z0);
   t->x0 = x0;
   t->z0 = z0;
   t->x1 = x1;
   t->z1 = z1;
   t->h = h;

   t->distortion = TER_WATER_DISTORTION;
   t->wave_speed = TER_WATER_WAVE_SPEED;
   t->reflection_factor = TER_WATER_REFLECTIVENESS;

   t->tile_size = tile_size;
   set_vertices(t);

   t->reflection = ter_render_texture_new(TER_WATER_REFLECTION_TEX_W,
                                          TER_WATER_REFLECTION_TEX_H,
                                          false, false);
   t->refraction = ter_render_texture_new(TER_WATER_REFRACTION_TEX_W,
                                          TER_WATER_REFRACTION_TEX_H,
                                          true, false);
   t->dudv_tex = dudv_tex;
   t->normal_tex = normal_tex;

   return t;
}

void
ter_water_tile_free(TerWaterTile *t)
{
   ter_render_texture_free(t->reflection);
   ter_render_texture_free(t->refraction);
   g_free(t->vertices);
   g_free(t);
}

void
ter_water_tile_update(TerWaterTile *t)
{
   t->wave_factor += t->wave_speed;
   if (t->wave_factor >= 1.0f)
      t->wave_factor -= 1.0f;
}

static void
water_bind_vao(TerWaterTile *t)
{
   if (t->vao == 0) {
      unsigned bytes = t->num_vertices * sizeof(glm::vec3);
      glGenBuffers(1, &t->vertex_buf);
      glBindBuffer(GL_ARRAY_BUFFER, t->vertex_buf);
      glBufferData(GL_ARRAY_BUFFER, bytes, &t->vertices[0], GL_STATIC_DRAW);

      glGenVertexArrays(1, &t->vao);
      glBindVertexArray(t->vao);

      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, t->vertex_buf);
      glVertexAttribPointer(
         0,                  // Attribute index
         3,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );

      ter_dbg(LOG_VBO,
              "WATER: VBO: INFO: Uploaded %u bytes (%u KB) "
              "for %u vertices (%u bytes/vertex)\n",
              bytes, bytes / 1024, t->num_vertices, bytes / t->num_vertices);
   } else {
      glBindVertexArray(t->vao);
      glEnableVertexAttribArray(0);
   }
}

static void
water_unbind(TerWaterTile *t)
{
   glBindVertexArray(0);
   glDisableVertexAttribArray(0);
}

void
ter_water_tile_render(TerWaterTile *t)
{
   TerShaderProgramWater *sh;
   sh = (TerShaderProgramWater *) ter_cache_get("program/water");
   glUseProgram(sh->basic.prog.program);

   glm::mat4 *Projection = (glm::mat4 *) ter_cache_get("matrix/Projection");
   glm::mat4 *View = (glm::mat4 *) ter_cache_get("matrix/View");
   glm::mat4 *ViewInv = (glm::mat4 *) ter_cache_get("matrix/ViewInv");
   glm::mat4 Model = glm::mat4(1.0);
   ter_shader_program_basic_load_MVP(&sh->basic,
                                     Projection, View, ViewInv, &Model);

   TerCamera *camera = (TerCamera *) ter_cache_get("camera/main");
   ter_shader_program_water_load_camera_position(sh, camera->pos);

   TerLight *light = (TerLight *) ter_cache_get("light/light0");
   ter_shader_program_basic_load_light(&sh->basic, light);
   ter_shader_program_basic_load_sky_color(&sh->basic, &light->diffuse);

   TerMaterial material;
   material.diffuse = glm::vec3(0.2f, 0.95f, 0.85f);
   material.ambient = glm::vec3(0.2f, 0.95f, 0.85f);
   material.specular = glm::vec3(0.2f, 0.6f, 0.6f);
   material.shininess = 32.0;
   ter_shader_program_basic_load_material(&sh->basic, &material);

   ter_shader_program_water_load_distortion_strength(sh, t->distortion);
   ter_shader_program_water_load_wave_factor(sh, t->wave_factor);
   ter_shader_program_water_load_reflection_factor(sh, t->reflection_factor);
   ter_shader_program_water_load_near_far_planes(sh,
                                                 TER_NEAR_PLANE, TER_FAR_PLANE);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, t->reflection->texture);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, t->refraction->texture);
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, t->dudv_tex);
   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_2D, t->normal_tex);
   glActiveTexture(GL_TEXTURE4);
   glBindTexture(GL_TEXTURE_2D, t->refraction->depth_texture);
   ter_shader_program_water_load_textures(sh, 0, 1, 2, 3, 4);

   TerShadowRenderer *sr =
      (TerShadowRenderer *) ter_cache_get("rendering/shadow-renderer");
   glm::mat4 ShadowMapSpaceVP =
      ter_shadow_renderer_get_shadow_map_space_vp(sr);
   glActiveTexture(GL_TEXTURE5);
   glBindTexture(GL_TEXTURE_2D, sr->shadow_map->map->depth_texture);
   ter_shader_program_shadow_data_load(
      &sh->shadow, &ShadowMapSpaceVP, TER_SHADOW_DISTANCE,
      TER_SHADOW_MAP_SIZE, TER_SHADOW_PFC, 5);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   water_bind_vao(t);
   glDrawArrays(GL_TRIANGLE_STRIP, 0, t->num_vertices);
   water_unbind(t);

   glDisable(GL_BLEND);
}
