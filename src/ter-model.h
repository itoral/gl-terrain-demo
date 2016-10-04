#ifndef __TER_MODEL_H__
#define __TER_MODEL_H__

#include <vector>
#include <glm/glm.hpp>

#include <glib.h>

#include "ter-shader-program.h"
#include "ter-util.h"
#include "ter-texture.h"

#define TER_MODEL_INSTANCED_ITEM_SIZE (16 * sizeof(float) + 1 * sizeof(int))
#define TER_MODEL_MAX_INSTANCED_OBJECTS (TER_MODEL_MAX_INSTANCED_VBO_BYTES / TER_MODEL_INSTANCED_ITEM_SIZE)

typedef struct {
   unsigned vao;
   unsigned vertex_buf;
   unsigned instanced_buf[TER_MODEL_NUM_INSTANCED_BUFFERS];
   /* Index of the currently active instanced buffer. The active instanced
    * buffer is the one to which we upload new data for subsequent draw
    * commands.
    */
   unsigned ibuf_idx;
   /* Number of instances that we have uploaded to the currently active instance
    * buffer by previous draw commands. When new instance data is uploaded, if
    * we have enough room in the currently active buffer, we upload it at the
    * end of the used region. If there is not enough space available in the
    * active buffer, then we select the next buffer in the array, upload the
    * data to it and mark it as the one currently active.
    *
    * Mapping larger buffers and using this tecnique is better for performance
    * than allocating smaller buffers and increasing the number of buffers. It
    * also makes better use of the memory allocated and allows us to render
    * more instances in a single draw call.
    */
   unsigned ibuf_used;

   std::vector<glm::vec3> vertices;
   std::vector<glm::vec2> uvs;
   std::vector<glm::vec3> normals;
   std::vector<int> mat_idx; /* material index */
   std::vector<int> samplers;

   TerMaterial materials[TER_MODEL_MAX_MATERIALS * TER_MODEL_MAX_VARIANTS];
   unsigned num_materials;
   unsigned tids[TER_MODEL_MAX_TEXTURES * TER_MODEL_MAX_VARIANTS];
   unsigned num_tids;
   unsigned num_variants;

   float w, h, d;
   glm::vec3 center;

   char *name;
} TerModel;

TerModel *ter_model_load_obj(const char *path);
void ter_model_free(TerModel *model);

void ter_model_bind_vao_for_shadow_map(TerModel *model);
void ter_model_render(TerModel *model,
                      glm::vec3 pos, glm::vec3 rot, glm::vec3 scale,
                      bool enable_shadow);

TerShaderProgramBasic *ter_model_render_prepare(TerModel *model,
                                                float *M4x4_list,
                                                unsigned num_instances,
                                                float clip_far_plane,
                                                float render_far_plane,
                                                bool enable_shadow,
                                                unsigned shadow_pfc);
void ter_model_render_prepare_for_shadow_map(TerModel *model,
                                             float *M4x4_list,
                                             unsigned num_instances);
void ter_model_render_finish(TerModel *model);
void ter_model_render_finish_for_shadow_map(TerModel *model);

bool ter_model_is_textured(TerModel *m);

void ter_model_add_variant(TerModel *m, TerMaterial *materials, unsigned *tids,
                           unsigned count);

#endif
