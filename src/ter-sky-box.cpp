#include "ter-sky-box.h"
#include "ter-shader-program.h"
#include "ter-camera.h"
#include "ter-cache.h"
#include "ter-texture.h"
#include "main-constants.h"

#include <glib.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

static const float vertices[] = {
   -1.0,  1.0,  -1.0,
   -1.0, -1.0,  -1.0,
    1.0, -1.0,  -1.0,
    1.0, -1.0,  -1.0,
    1.0,  1.0,  -1.0,
   -1.0,  1.0,  -1.0,

   -1.0, -1.0,   1.0,
   -1.0, -1.0,  -1.0,
   -1.0,  1.0,  -1.0,
   -1.0,  1.0,  -1.0,
   -1.0,  1.0,   1.0,
   -1.0, -1.0,   1.0,

    1.0, -1.0,  -1.0,
    1.0, -1.0,   1.0,
    1.0,  1.0,   1.0,
    1.0,  1.0,   1.0,
    1.0,  1.0,  -1.0,
    1.0, -1.0,  -1.0,

   -1.0, -1.0,   1.0,
   -1.0,  1.0,   1.0,
    1.0,  1.0,   1.0,
    1.0,  1.0,   1.0,
    1.0, -1.0,   1.0,
   -1.0, -1.0,   1.0,

   -1.0,  1.0,  -1.0,
    1.0,  1.0,  -1.0,
    1.0,  1.0,   1.0,
    1.0,  1.0,   1.0,
   -1.0,  1.0,   1.0,
   -1.0,  1.0,  -1.0,

   -1.0, -1.0,  -1.0,
   -1.0, -1.0,   1.0,
    1.0, -1.0,  -1.0,
    1.0, -1.0,  -1.0,
   -1.0, -1.0,   1.0,
    1.0, -1.0,   1.0,
};

TerSkyBox *
ter_skybox_new(unsigned size, unsigned vtid)
{
   TerSkyBox *b = g_new0(TerSkyBox, 1);
   b->size = size;
   b->vtid = vtid;

   return b;
}

void
ter_skybox_free(TerSkyBox *b)
{
   glDeleteVertexArrays(1, &b->vao);
   glDeleteBuffers(1, &b->vertex_buf);
   g_free(b);
}

void
ter_skybox_update(TerSkyBox *b)
{
   b->rot.y += b->speed;
}

static void
skybox_bind_vao(TerSkyBox *b)
{
   if (b->vao == 0) {
      glGenBuffers(1, &b->vertex_buf);
      glBindBuffer(GL_ARRAY_BUFFER, b->vertex_buf);
      glBufferData(GL_ARRAY_BUFFER,
                   6 * 6 * 3 * sizeof(float), vertices, GL_STATIC_DRAW);

      glGenVertexArrays(1, &b->vao);
      glBindVertexArray(b->vao);

      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, b->vertex_buf);
      glVertexAttribPointer(
         0,                  // Attribute index
         3,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         0,                  // stride
         (void*)0            // array buffer offset
      );
   } else {
      glBindVertexArray(b->vao);
   }
}

void
ter_skybox_render(TerSkyBox *b)
{
   TerShaderProgramSkybox *sh =
      (TerShaderProgramSkybox *) ter_cache_get("program/skybox");
   glUseProgram(sh->basic.prog.program);

   glm::mat4 *Projection = (glm::mat4 *) ter_cache_get("matrix/ProjectionSky");

   /* We want the skybox to move with the camera, so it seems fixed in the
    * distance from the camera's perspective. We can do that by removing
    * the camera translation transform from the View matrix.
    */
   glm::mat4 *View = (glm::mat4 *) ter_cache_get("matrix/View");
   glm::mat4 FixedView, FixedViewInv;
   TerCamera *cam = (TerCamera *) ter_cache_get("camera/main");
   FixedView = glm::translate(*View, cam->pos);
   FixedViewInv = glm::inverse(FixedView);
   glm::mat4 Model = glm::mat4(1.0);
   Model = glm::scale(Model, glm::vec3(b->size));
   Model = glm::rotate(Model, DEG_TO_RAD(b->rot.x), glm::vec3(1, 0, 0));
   Model = glm::rotate(Model, DEG_TO_RAD(b->rot.y), glm::vec3(0, 1, 0));
   Model = glm::rotate(Model, DEG_TO_RAD(b->rot.z), glm::vec3(0, 0, 1));
   ter_shader_program_basic_load_MVP(&sh->basic,
                                     Projection, &FixedView, &FixedViewInv, &Model);

   TerTextureManager *tex_mgr =
      (TerTextureManager *) ter_cache_get("textures/manager");
   unsigned tid = ter_texture_manager_get_tid(tex_mgr, TER_TEX_SKY_BOX_01);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_CUBE_MAP, tid);
   ter_shader_program_skybox_load_sampler(sh, 0);

   skybox_bind_vao(b);
   glDrawArrays(GL_TRIANGLES, 0, 36);
}

