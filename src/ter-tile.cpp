#include "ter-tile.h"
#include "ter-cache.h"
#include "ter-shader-program.h"

#include <glib.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

static void
set_vertices_and_uvs(TerTile *t, unsigned width, unsigned height)
{
   t->vertices[0] = glm::vec2(0.0f,  0.0f);
   t->vertices[1] = glm::vec2(width, 0.0f);
   t->vertices[2] = glm::vec2(0.0f,  height);
   t->vertices[3] = glm::vec2(0.0f,  height);
   t->vertices[4] = glm::vec2(width, 0.0f);
   t->vertices[5] = glm::vec2(width, height);

   t->uvs[0] = glm::vec2(0.0f, 0.0f);
   t->uvs[1] = glm::vec2(1.0f, 0.0f);
   t->uvs[2] = glm::vec2(0.0f, 1.0f);
   t->uvs[3] = glm::vec2(0.0f, 1.0f);
   t->uvs[4] = glm::vec2(1.0f, 0.0f);
   t->uvs[5] = glm::vec2(1.0f, 1.0f);
}

TerTile *
ter_tile_new(unsigned width, unsigned height, float x, float y,
             unsigned texture, unsigned sampler)
{
   TerTile *t = g_new0(TerTile, 1);
   t->width = width;
   t->height = height;

   set_vertices_and_uvs(t, width, height);

   t->pos = glm::vec3(x, y, 0.0f);
   t->scale = glm::vec3(1.0f, 1.0f, 1.0f);
   t->texture = texture;
   t->sampler = sampler;

   return t;
}

static void
tile_bind_vao(TerTile *t)
{
   if (t->vao == 0) {
      glGenBuffers(1, &t->vertex_buf);
      glBindBuffer(GL_ARRAY_BUFFER, t->vertex_buf);
      glBufferData(GL_ARRAY_BUFFER,
                   6 * sizeof(glm::vec2),
                   &t->vertices[0],
                   GL_STATIC_DRAW);

      glGenBuffers(1, &t->uv_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, t->uv_buffer);
      glBufferData(GL_ARRAY_BUFFER,
                   6 * sizeof(glm::vec2),
                   &t->uvs[0],
                   GL_STATIC_DRAW);

      glGenVertexArrays(1, &t->vao);
      glBindVertexArray(t->vao);

      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, t->vertex_buf);
      glVertexAttribPointer(
         0,                  // Attribute index
         2,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );

      glEnableVertexAttribArray(1);
      glBindBuffer(GL_ARRAY_BUFFER, t->uv_buffer);
      glVertexAttribPointer(
         1,                  // Attribute index
         2,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );
   } else {
      glBindVertexArray(t->vao);
   }
}

void
ter_tile_render(TerTile *tile)
{
   TerShaderProgramTile *sh;
   sh = (TerShaderProgramTile *) ter_cache_get("program/tile");
   glUseProgram(sh->prog.program);

   glm::mat4 *Projection = (glm::mat4 *) ter_cache_get("matrix/ProjectionOrtho");
   glm::mat4 Model = glm::mat4(1.0);
   Model = glm::translate(Model, tile->pos);
   Model = glm::scale(Model, tile->scale);
   Model = glm::rotate(Model, DEG_TO_RAD(tile->rot.x), glm::vec3(1, 0, 0));
   Model = glm::rotate(Model, DEG_TO_RAD(tile->rot.y), glm::vec3(0, 1, 0));
   Model = glm::rotate(Model, DEG_TO_RAD(tile->rot.z), glm::vec3(0, 0, 1));
   ter_shader_program_tile_load_MVP(sh, Projection, &Model);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, tile->texture);
   glBindSampler(0, tile->sampler);
   ter_shader_program_tile_load_texture(sh, 0);

   tile_bind_vao(tile);
   glDrawArrays(GL_TRIANGLES, 0, 6);
}
