#include "ter-model.h"

#include <stdio.h>
#include <string.h>
#include <glib.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

#include <glm/gtc/type_ptr.hpp>

#include "ter-cache.h"
#include "ter-shadow-renderer.h"
#include "main-constants.h"

#define TER_MODEL_ENABLE_DEBUG true
#define NUM_VERTEX_ATTRIBS_SOLID     8
#define NUM_VERTEX_ATTRIBS_TEXTURED 10

static TerModel *
model_new()
{
   TerModel *m = g_new0(TerModel, 1);
   m->num_variants = 1;
   return m;
}

static inline bool
model_is_textured(TerModel *model)
{
   return model->uvs.size() != 0;
}

static void
model_compute_dimensions(TerModel *m)
{
   float min_x, max_x, min_y, max_y, max_z, min_z;

   min_x = max_x = m->vertices[0].x;
   min_y = max_y = m->vertices[0].y;
   min_z = max_z = m->vertices[0].z;
   for (unsigned i = 1; i < m->vertices.size(); i++) {
      if (m->vertices[i].x < min_x)
         min_x = m->vertices[i].x;
      else if (m->vertices[i].x > max_x)
         max_x = m->vertices[i].x;

      if (m->vertices[i].y < min_y)
         min_y = m->vertices[i].y;
      else if (m->vertices[i].y > max_y)
         max_y = m->vertices[i].y;

      if (m->vertices[i].z < min_z)
         min_z = m->vertices[i].z;
      else if (m->vertices[i].z > max_z)
         max_z = m->vertices[i].z;
   }

   m->w = max_x - min_x;
   m->h = max_y - min_y;
   m->d = max_z - min_z;

   m->center.x = (max_x + min_x) / 2.0f;
   m->center.y = (max_y + min_y) / 2.0f;
   m->center.z = (max_z + min_z) / 2.0f;
}

static bool
read_vertex_mapping(const char *buffer, int line,
                    bool has_uvs, bool has_normals,
                    unsigned *vertex_index,
                    unsigned *uv_index,
                    unsigned *normal_index)
{
   int matches;

   if (has_uvs && has_normals) {
      matches = sscanf(buffer, "%d/%d/%d %d/%d/%d %d/%d/%d",
                       &vertex_index[0], &uv_index[0], &normal_index[0],
                       &vertex_index[1], &uv_index[1], &normal_index[1],
                       &vertex_index[2], &uv_index[2], &normal_index[2]);
      if (matches != 9) {
         ter_dbg(LOG_OBJ_LOAD,
                 "OBJ-LOADER: ERROR: read %d vertex mappings, expected 9 "
                 "[line %d]\n", matches, line);
         return false;
      }
   } else if (has_uvs || has_normals) {
      unsigned *sec_index = has_uvs ? uv_index : normal_index;
      matches = sscanf(buffer, "%d/%d %d/%d %d/%d",
                       &vertex_index[0], &sec_index[0],
                       &vertex_index[1], &sec_index[1],
                       &vertex_index[2], &sec_index[2]);
      if (matches != 6) {
         const char *fmt = (!has_uvs) ?
            "%d//%d %d//%d %d//%d" : "%d/%d/ %d/%d/ %d/%d/";
         matches = sscanf(buffer, fmt,
                          &vertex_index[0], &sec_index[0],
                          &vertex_index[1], &sec_index[1],
                          &vertex_index[2], &sec_index[2]);
         if (matches != 6) {
            ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: ERROR: read %d vertex mappings, expected 6 "
                    "[line %d]\n", matches, line);
            return false;
         }
      }
   } else {
      matches = sscanf(buffer, "%d %d %d",
                       &vertex_index[0], &vertex_index[1], &vertex_index[2]);
      if (matches != 3) {
         ter_dbg(LOG_OBJ_LOAD,
                 "OBJ-LOADER: ERROR: read %d vertex mappings, expected 3 "
                 "[line %d]\n", matches, line);
         return false;
      }
   }

   return true;
}

static bool
load_materials(const char *path, TerMaterial *materials, unsigned *mat_count,
               unsigned *tids, unsigned *num_tids, GHashTable *mat_names)
{
   ter_dbg(LOG_OBJ_LOAD,
           "OBJ-LOADER: INFO: loading materials from: '%s'\n", path);

   int line = 0;
   char line_header[512];
   char line_buffer[512];
   char *buffer;
   TerMaterial *mat = NULL;
   TerTextureManager *texmgr = NULL;
   FILE *file = NULL;
   bool res = true;

   *mat_count = 0;
   *num_tids = 0;

   file = fopen(path, "r");
   if (file == NULL) {
      res = false;
      goto cleanup;
   }

   while (true) {
      line++;

      if (fgets(line_buffer, 512, file) == NULL)
         break;

      /* Skip empty lines */
      if (sscanf(line_buffer, "%s", line_header) != 1)
         continue;

      buffer = g_strchug(line_buffer + strlen(line_header));

      /* New material */
      if (strcmp(line_header, "newmtl") == 0) {
         if (*mat_count >= TER_MODEL_MAX_MATERIALS) {
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: ERROR: too many materials [line %d]\n", line);
            res = false;
            goto cleanup;
         }

         char name[128];
         if (sscanf(buffer, "%s\n", name) != 1) {
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: ERROR: bogus newmtl [line %d]\n", line);
            res = false;
            goto cleanup;
         }

         ter_dbg(LOG_OBJ_LOAD,
                 "\tOBJ-LOADER: INFO: loading material: '%s'\n", name);
         g_hash_table_insert(mat_names,
                             g_strdup(name), GINT_TO_POINTER(*mat_count));
         mat = &materials[(*mat_count)++];
      }

      /* Shininess */
      else if (strcmp(line_header, "Ns") == 0) {
         float s;
         if (sscanf(buffer, "%f\n", &s) != 1) {
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: ERROR: bogus Ns [line %d]\n", line);
            res = false;
            goto cleanup;
         }
         mat->shininess = s;
      }

      /* Ambient */
      else if (strcmp(line_header, "Ka") == 0) {
         glm::vec3 a;
         if (sscanf(buffer, "%f %f %f\n", &a.x, &a.y, &a.z) != 3) {
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: ERROR: bogus Ka [line %d]\n", line);
            res = false;
            goto cleanup;
         }
         mat->ambient = a;
      }

      /* Diffuse */
      else if (strcmp(line_header, "Kd") == 0) {
         glm::vec3 d;
         if (sscanf(buffer, "%f %f %f\n", &d.x, &d.y, &d.z) != 3) {
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: ERROR: bogus Kd [line %d]\n", line);
            res = false;
            goto cleanup;
         }
         mat->diffuse = d;
      }

      /* Specular */
      else if (strcmp(line_header, "Ks") == 0) {
         glm::vec3 s;
         if (sscanf(buffer, "%f %f %f\n", &s.x, &s.y, &s.z) != 3) {
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: ERROR: bogus Ks [line %d]\n", line);
            res = false;
            goto cleanup;
         }
         mat->specular = s;
      }

      /* Diffuse texture */
      else if (strcmp(line_header, "map_Kd") == 0) {
         char tex_file[256];
         if (sscanf(buffer, "%s\n", tex_file) != 1) {
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: ERROR: bogus map_Kd [line %d]\n", line);
            res = false;
            goto cleanup;
         }

         if (*num_tids >= TER_MODEL_MAX_TEXTURES) {
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: ERROR: too many material textures (limit=%d) "
                    "[line %d]\n", TER_MODEL_MAX_TEXTURES, line);
            res = false;
            goto cleanup;
         }

         char *dir = g_path_get_dirname(path);
         char *tex_path = g_strconcat(dir, "/", tex_file, NULL);

         if (texmgr == NULL)
            texmgr = ter_texture_manager_new(1);
         mat->tid = ter_texture_manager_load(texmgr, tex_path, 0);
         tids[(*num_tids)++] = mat->tid;
         ter_dbg(LOG_OBJ_LOAD,
                 "\tOBJ-LOADER: INFO: loaded material texture: '%s'\n", tex_path);
         g_free(dir);
         g_free(tex_path);
      }

      /* Unknown */
      else {
         if (line_header[0] != '#')
            ter_dbg(LOG_OBJ_LOAD,
                    "\tOBJ-LOADER: WARNING: unknown material header: "
                    "%s [line %d]\n", line_header, line);
      }
   }

cleanup:
   if (texmgr)
      ter_texture_manager_free_nogl(texmgr);
   if (file)
      fclose(file);
   return res;
}

TerModel *
ter_model_load_obj(const char *path)
{
   unsigned matches;
   bool has_uvs = false;
   bool has_normals = false;
   bool has_materials = false;
   int line = 0;
   int num_objects = 0;
   int cur_material = -1;

   std::vector<unsigned> vertex_indices, uv_indices, normal_indices;
   std::vector<glm::vec3> temp_vertices; 
   std::vector<glm::vec2> temp_uvs;
   std::vector<glm::vec3> temp_normals;
   unsigned tids[TER_MODEL_MAX_TEXTURES];
   unsigned tid_count = 0;
   GHashTable *mat_names = NULL;
   char *base_path = NULL;
   char *suffix = NULL;
   TerModel *m = model_new();;

   ter_dbg(LOG_OBJ_LOAD,
           "OBJ-LOADER: INFO: loading model from '%s'\n", path);

   FILE *file = fopen(path, "r");
   if (file == NULL){
      ter_dbg(LOG_OBJ_LOAD,
              "OBJ-LOADER: ERROR:could not open model file '%s'\n", path);
      goto cleanup;
   }

   while (true) {
      char line_header[512];
      char line_buffer[512];
      char *buffer;

      line++;
      if (fgets(line_buffer, 512, file) == NULL)
         break;

      /* Skip empty lines */
      if (sscanf(line_buffer, "%s", line_header) != 1)
         continue;

      buffer = g_strchug(line_buffer + strlen(line_header));

      /* Materials file */
      if (strcmp(line_header, "mtllib") == 0) {
         char mtl_file[512];
         matches = sscanf(buffer, "%s\n", mtl_file);
         if (matches != 1) {
            ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: ERROR: bogus mtllib [line %d]\n", line);
            goto cleanup;
         }

         mat_names =
            g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

         char *dir = g_path_get_dirname(path);
         char *mtl_path = g_strconcat(dir, "/", mtl_file, NULL);
         bool res = load_materials(mtl_path, m->materials, &m->num_materials,
                                   tids, &tid_count, mat_names);
         g_free(dir);
         g_free(mtl_path);
         if (!res) {
            ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: ERROR: failed to load materials [line %d]\n",
                    line);
            goto cleanup;
         }
      }

      /* Current material */
      else if (strcmp(line_header, "usemtl") == 0) {
         char name[128];
         matches = sscanf(buffer, "%s\n", name);
         if (matches != 1) {
            ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: ERROR: bogus usemtl [line %d]\n", line);
            goto cleanup;
         }
         cur_material = GPOINTER_TO_INT(g_hash_table_lookup(mat_names, name));
         has_materials = true;
      }

      /* Vertex coordinate */
      else if (strcmp(line_header, "v") == 0) {
         glm::vec3 vertex;
         matches =
            sscanf(buffer, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
         if (matches != 3) {
            ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: ERROR: Read %d vertex components, expected 3 "
                    "[line %d]\n", matches, line);
            goto cleanup;
         }
         temp_vertices.push_back(vertex);
      }

      /* Texture coordinate */
      else if (strcmp(line_header, "vt") == 0 ) {
         glm::vec2 uv;
         matches = sscanf(buffer, "%f %f\n", &uv.x, &uv.y);
         if (matches != 2) {
            float tmp;
            matches = sscanf(buffer, "%f %f %f\n", &uv.x, &uv.y, &tmp);
            if (matches != 3) {
               ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: ERROR: Read %d texture coordinates, expected 2 "
                    "[line %d]\n", matches, line);
               goto cleanup;
            }
         }
         temp_uvs.push_back(uv);
         has_uvs = true;
      }

      /* Vertex normal */
      else if (strcmp(line_header, "vn") == 0 ) {
         glm::vec3 normal;
         matches = sscanf(buffer, "%f %f %f\n",
                          &normal.x, &normal.y, &normal.z);
         if (matches != 3) {
            ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: ERROR: Read %d vertex normal components, expected 3 "
                    "[line %d]\n", matches, line);
            goto cleanup;
         }
         temp_normals.push_back(normal);
         has_normals = true;
      }

      /* Vertex mapping */
      else if (strcmp(line_header, "f") == 0 ) {
         unsigned vertex_index[3];
         unsigned uv_index[3];
         unsigned normal_index[3];

         read_vertex_mapping(buffer, line, has_uvs, has_normals,
                             vertex_index, uv_index, normal_index);

         vertex_indices.push_back(vertex_index[0]);
         vertex_indices.push_back(vertex_index[1]);
         vertex_indices.push_back(vertex_index[2]);

         if (has_uvs) {
            uv_indices.push_back(uv_index[0]);
            uv_indices.push_back(uv_index[1]);
            uv_indices.push_back(uv_index[2]);
         }

         if (has_normals) {
			   normal_indices.push_back(normal_index[0]);
			   normal_indices.push_back(normal_index[1]);
			   normal_indices.push_back(normal_index[2]);
         }

         if (has_materials) {
            if (cur_material < 0 || cur_material >= (int) m->num_materials) {
               ter_dbg(LOG_OBJ_LOAD,
                       "OBJ-LOADER: ERROR: bogus current material for vertex mapping "
                       "[line %d]\n", line);
               goto cleanup;
            }
            m->mat_idx.push_back(cur_material);
            m->mat_idx.push_back(cur_material);
            m->mat_idx.push_back(cur_material);

            /* If this is a textured model we need to identify the sampler
             * units that will be used with each vertex
             */
            unsigned sampler_index = 0;
            for (; sampler_index < tid_count; sampler_index++) {
               if (tids[sampler_index] == m->materials[cur_material].tid) {
                  m->samplers.push_back(sampler_index);
                  m->samplers.push_back(sampler_index);
                  m->samplers.push_back(sampler_index);
                  break;
               }
            }

            if (tid_count > 0 && sampler_index >= tid_count) {
               /* Mmm... we have a textured model, but this vertex's material
                * is not textured, we don't really support this at the moment
                */
               ter_dbg(LOG_OBJ_LOAD,
                       "OBJ-LOADER: ERROR: model must be fully textured or have no "
                       "textures\n");
               goto cleanup;
            }
         }
      }

      /* Object start */
      else if (strcmp(line_header, "o") == 0) {
         num_objects++;
         if (num_objects > 1) {
            ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: ERROR: parser only supports one object per model "
                    "[line %d]\n", line);
            goto cleanup;
         }
      }

      /* Unknown */
      else {
         if (line_header[0] != '#')
            ter_dbg(LOG_OBJ_LOAD,
                    "OBJ-LOADER: WARNING: unknown header: %s "
                    "[line %d]\n", line_header, line);
      }
   }

   /* Create vertex / uv / normal / material lists from indices */
   for (unsigned int i = 0; i < vertex_indices.size(); i++) {
      unsigned vertex_index = vertex_indices[i];
      glm::vec3 vertex = temp_vertices[vertex_index - 1];
      m->vertices.push_back(vertex);

      if (has_uvs) {
         unsigned uv_index = uv_indices[i];
         glm::vec2 uv = temp_uvs[uv_index - 1];
         m->uvs.push_back(uv);
      }

      if (has_normals) {
         unsigned normal_index = normal_indices[i];
         glm::vec3 normal = temp_normals[normal_index - 1];
         m->normals.push_back(normal);
      }
   }

   m->num_tids = tid_count;
   for (unsigned i = 0; i < tid_count; i++)
      m->tids[i] = tids[i];

   /* Use the base name of the file (without suffix) as the model name */
   base_path = g_strrstr(path, "/");
   if (base_path)
      base_path += 1;
   else
      base_path = (char *) path;
   suffix = g_strrstr(base_path, ".");
   if (suffix)
      m->name = g_strndup(base_path, suffix - base_path);
   else
      m->name = g_strdup(base_path);

   model_compute_dimensions(m);

   ter_dbg(LOG_OBJ_LOAD,
           "OBJ-LOADER: INFO: Loaded model '%s'. "
           "Vertices: %d (%d triangles), "
           "Materials: %d, Texture Coords: %s, Num Textures: %d, "
           "Normals: %s\n",
           m->name,
           (int) m->vertices.size(), (int) m->vertices.size() / 3,
           m->num_materials, has_uvs ? "Yes" : "No", tid_count,
           has_normals ? "Yes" : "No");


   g_hash_table_unref(mat_names);
   if (file)
      fclose(file);

   return m;

cleanup:
   g_hash_table_unref(mat_names);
   if (file)
      fclose(file);
   ter_model_free(m);
   return m;
}

static void
upload_and_bind_vertex_data(TerModel *model, float *M4x4_list,
                            unsigned num_instances)
{
   unsigned vert_count = model->vertices.size();
   assert(model->normals.size() == vert_count);
   assert(model->mat_idx.size() == vert_count);
   assert(!model_is_textured(model) || model->uvs.size() == vert_count);

   bool is_textured = model_is_textured(model);

   /* Interleave attributes for better performance on some platforms.
    * We need to allocate a buffer large enough to store all attribute data for
    * each vertex considering whether it is textured or not.
    */
   unsigned position_size = sizeof(glm::vec3);
   unsigned normal_size = sizeof(glm::vec3);
   unsigned mat_idx_size = sizeof(int);
   unsigned uv_size = 0;
   unsigned sampler_size = 0;
   unsigned num_attrs = NUM_VERTEX_ATTRIBS_SOLID;
   if (is_textured) {
      uv_size = sizeof(glm::vec2);
      sampler_size = sizeof(int);
      num_attrs = NUM_VERTEX_ATTRIBS_TEXTURED;
   }

   unsigned vertex_byte_size =
      position_size + normal_size + mat_idx_size + uv_size + sampler_size;

   unsigned bytes = vert_count * vertex_byte_size;
   uint8_t *vertex_data = g_new(uint8_t, bytes);

   for (unsigned i = 0; i < vert_count; i++) {
      unsigned offset = 0;

      memcpy(&vertex_data[vertex_byte_size * i + offset],
             &model->vertices[i], position_size);
      offset += position_size;

      memcpy(&vertex_data[vertex_byte_size * i + offset],
             &model->normals[i], normal_size);
      offset += normal_size;

      memcpy(&vertex_data[vertex_byte_size * i + offset],
             &model->mat_idx[i], mat_idx_size);
      offset += mat_idx_size;

      if (is_textured) {
         memcpy(&vertex_data[vertex_byte_size * i + offset],
                &model->uvs[i], uv_size);
         offset += uv_size;

         memcpy(&vertex_data[vertex_byte_size * i + offset],
                &model->samplers[i], sampler_size);
         offset += sampler_size;
      }
   }

   /* Upload non-mutable vertex buffer */
   glGenBuffers(1, &model->vertex_buf);
   glBindBuffer(GL_ARRAY_BUFFER, model->vertex_buf);
   glBufferData(GL_ARRAY_BUFFER, bytes, vertex_data, GL_STATIC_DRAW);
   g_free(vertex_data);

   /* Upload instanced data to the first instanced buffer and allocate buffer
    * storage for the other buffers
    */
   glGenBuffers(TER_MODEL_NUM_INSTANCED_BUFFERS, model->instanced_buf);
   for (int i = 0; i < TER_MODEL_NUM_INSTANCED_BUFFERS; i++) {
      glBindBuffer(GL_ARRAY_BUFFER, model->instanced_buf[i]);
      glBufferData(GL_ARRAY_BUFFER,
                   TER_MODEL_MAX_INSTANCED_VBO_BYTES,
                   i == 0 ? &M4x4_list[0] : NULL,
                   GL_DYNAMIC_DRAW);
   }

   glGenVertexArrays(1, &model->vao);
   glBindVertexArray(model->vao);

   /* Model matrix (attribute locations 1-4) */
   model->ibuf_idx = 0;
   glBindBuffer(GL_ARRAY_BUFFER, model->instanced_buf[model->ibuf_idx]);
   for (int i = 1, c = 0; i < 5; i++, c++) {
      glEnableVertexAttribArray(i);
      glVertexAttribPointer(
         i,                  // Attribute index
         4,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         TER_MODEL_INSTANCED_ITEM_SIZE, // stride
         (void*)(c * 4 * sizeof(float)) // array buffer offset
      );
      glVertexAttribDivisor(i, 1);
   }

   glEnableVertexAttribArray(5);
   glVertexAttribIPointer(
      5,                     // Attribute index
      1,                     // size
      GL_INT,                // type
      TER_MODEL_INSTANCED_ITEM_SIZE, // stride
      (void*)(16 * sizeof(float))    // array buffer offset
   );
   glVertexAttribDivisor(5, 1);

   model->ibuf_used = num_instances;

   /* Unmutable interleaved data */
   glBindBuffer(GL_ARRAY_BUFFER, model->vertex_buf);

   /* Position */
   size_t offset = 0;
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(
      0,                  // Attribute index
      3,                  // size
      GL_FLOAT,           // type
      GL_FALSE,           // normalized?
      vertex_byte_size,   // stride
      (void*)offset       // array buffer offset
   );
   offset += position_size;

   /* Normals */
   unsigned attr_index = 6;
   glEnableVertexAttribArray(attr_index);
   glVertexAttribPointer(
      attr_index++,       // Attribute index
      3,                  // size
      GL_FLOAT,           // type
      GL_FALSE,           // normalized?
      vertex_byte_size,   // stride
      (void*)offset       // array buffer offset
   );
   offset += normal_size;

   /* Material index */
   glEnableVertexAttribArray(attr_index);
   glVertexAttribIPointer(
      attr_index++,       // Attribute index
      1,                  // size
      GL_INT,             // type
      vertex_byte_size,   // stride
      (void*)offset       // array buffer offset
   );
   offset += mat_idx_size;

   if (is_textured) {
      /* UVs */
      glEnableVertexAttribArray(attr_index);
      glVertexAttribPointer(
         attr_index++,       // Attribute index
         2,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         vertex_byte_size,   // stride
         (void*)offset       // array buffer offset
      );
      offset += uv_size;

      /* Sampler index */
      glEnableVertexAttribArray(attr_index);
      glVertexAttribIPointer(
         attr_index++,       // Attribute index
         1,                  // size
         GL_INT,             // type
         vertex_byte_size,   // stride
         (void*)offset       // array buffer offset
      );
      offset += sampler_size;
   }

   ter_dbg(LOG_VBO, "MODEL(%s): VBO: INFO: Uploaded %u bytes (%u KB) "
           "for %u vertices (%u bytes/vertex)\n",
           model->name, bytes, bytes / 1024, vert_count, vertex_byte_size);

   ter_dbg(LOG_VBO, "MODEL(%s): VBO: INFO: Created %u bytes (%u KB) buffer for "
           "instanced data\n", model->name, TER_MODEL_MAX_INSTANCED_VBO_BYTES,
           TER_MODEL_MAX_INSTANCED_VBO_BYTES / 1024);

   assert(attr_index == num_attrs);
   assert(offset == vertex_byte_size);
}

static void inline
upload_instanced_data_and_bind(TerModel *model,
                               float *M4x4_list, unsigned num_instances)
{
   /* Upload the new instance data */
   unsigned buffer_offset = 0;

   unsigned ibuf_available = TER_MODEL_MAX_INSTANCED_OBJECTS - model->ibuf_used;
   if (num_instances > ibuf_available) {
      model->ibuf_idx++;
      if (model->ibuf_idx >= TER_MODEL_NUM_INSTANCED_BUFFERS)
         model->ibuf_idx = 0;
      model->ibuf_used = num_instances;
   } else {
      buffer_offset = model->ibuf_used * TER_MODEL_INSTANCED_ITEM_SIZE;
      model->ibuf_used += num_instances;
   }

   unsigned bytes = num_instances * TER_MODEL_INSTANCED_ITEM_SIZE;
   glBindBuffer(GL_ARRAY_BUFFER, model->instanced_buf[model->ibuf_idx]);
   glBufferSubData(GL_ARRAY_BUFFER, buffer_offset, bytes, &M4x4_list[0]);

   ter_dbg(LOG_VBO,
           "MODEL(%s): VBO: INFO: Updated %u bytes (%u KB) of instanced data "
           "for %d instances (buf=%u, off=%u)\n",
           model->name, bytes, bytes / 1024, num_instances,
           model->ibuf_idx, buffer_offset);

   /* Bind the VAO and re-configure the instanced attributes to read from
    * the correct buffer and offset where we have just uploaded the data
    */
   glBindVertexArray(model->vao);
   glBindBuffer(GL_ARRAY_BUFFER, model->instanced_buf[model->ibuf_idx]);
   for (int i = 1, c = 0; i < 5; i++, c++) {
      glEnableVertexAttribArray(i);
      glVertexAttribPointer(
         i,                  // Attribute index
         4,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         TER_MODEL_INSTANCED_ITEM_SIZE, // stride
         (void*)(buffer_offset + c * 4 * sizeof(float)) // array buffer offset
      );
      glVertexAttribDivisor(i, 1);
   }

   glEnableVertexAttribArray(5);
   glVertexAttribIPointer(
      5,                     // Attribute index
      1,                     // size
      GL_INT  ,              // type
      TER_MODEL_INSTANCED_ITEM_SIZE, // stride
      (void*)(buffer_offset + 16 * sizeof(float)) // array buffer offset
   );
   glVertexAttribDivisor(5, 1);
}

static void
model_bind_vao(TerModel *model, float *M4x4_list, unsigned num_instances)
{
   if (model->vao == 0) {
      upload_and_bind_vertex_data(model, M4x4_list, num_instances);
   } else {
      upload_instanced_data_and_bind(model, M4x4_list, num_instances);
      unsigned num_attrs = model_is_textured(model) ?
         NUM_VERTEX_ATTRIBS_TEXTURED : NUM_VERTEX_ATTRIBS_SOLID;
      for (unsigned i = 0; i < num_attrs; i++)
         glEnableVertexAttribArray(i);
   }
}

static void
model_unbind(TerModel *model)
{
   bool is_textured = model_is_textured(model);

   if (is_textured)
      glBindTexture(GL_TEXTURE_2D, 0);

   unsigned num_attrs =  is_textured ?
      NUM_VERTEX_ATTRIBS_TEXTURED : NUM_VERTEX_ATTRIBS_SOLID;
   for (unsigned i = 0; i < num_attrs; i++)
      glDisableVertexAttribArray(i);

   glBindVertexArray(0);
}

void
ter_model_free(TerModel *m)
{
   if (!m)
      return;

   g_free(m->name);

   m->vertices.clear();
   std::vector<glm::vec3>(m->vertices).swap(m->vertices);
   m->uvs.clear();
   std::vector<glm::vec2>(m->uvs).swap(m->uvs);
   m->normals.clear();
   std::vector<glm::vec3>(m->normals).swap(m->normals);
   m->mat_idx.clear();
   std::vector<int>(m->mat_idx).swap(m->mat_idx);
   m->samplers.clear();
   std::vector<int>(m->samplers).swap(m->samplers);

   glDeleteVertexArrays(1, &m->vao);
   glDeleteBuffers(1, &m->vertex_buf);
   glDeleteBuffers(TER_MODEL_NUM_INSTANCED_BUFFERS, m->instanced_buf);

   g_free(m);
}

static TerShaderProgramBasic *
get_shader_program(TerModel *model, bool enable_shadow, bool *is_solid)
{
   *is_solid = model->uvs.size() == 0;

   TerShaderProgramBasic *sh;
   if (!enable_shadow) {
      if (*is_solid)
         sh = (TerShaderProgramBasic *) ter_cache_get("program/model-solid");
      else
         sh = (TerShaderProgramBasic *) ter_cache_get("program/model-tex");
   } else {
      if (*is_solid)
         sh = (TerShaderProgramBasic *)
            ter_cache_get("program/model-solid-shadow");
      else
         sh = (TerShaderProgramBasic *)
            ter_cache_get("program/model-tex-shadow");
   }

   return sh;
}

static inline TerShaderProgramShadowData *
get_shader_program_shadow_data(TerShaderProgramBasic *p, bool is_solid)
{
   if (is_solid) {
      return &((TerShaderProgramModelSolid *) p)->shadow;
   } else {
      return &((TerShaderProgramModelTex *) p)->shadow;
   }
}

void
ter_model_render(TerModel *model,
                 glm::vec3 pos, glm::vec3 rot, glm::vec3 scale,
                 bool enable_shadow)
{
   glm::mat4 Model = glm::mat4(1.0);
   Model = glm::translate(Model, pos);
   Model = glm::scale(Model, scale);
   Model = glm::rotate(Model, DEG_TO_RAD(rot.x), glm::vec3(1, 0, 0));
   Model = glm::rotate(Model, DEG_TO_RAD(rot.y), glm::vec3(0, 1, 0));
   Model = glm::rotate(Model, DEG_TO_RAD(rot.z), glm::vec3(0, 0, 1));
   float *Model_fptr = glm::value_ptr(Model);

   ter_model_render_prepare(model, (float *) Model_fptr, 1,
                            TER_FAR_PLANE, TER_FAR_PLANE,
                            TER_SHADOW_PFC, enable_shadow);

   glDrawArraysInstanced(GL_TRIANGLES, 0, model->vertices.size(), 1);

   ter_model_render_finish(model);
}

TerShaderProgramBasic *
ter_model_render_prepare(TerModel *model,
                         float *M4x4_list, unsigned num_instances,
                         float clip_far_plane, float render_far_plane,
                         bool enable_shadow, unsigned shadow_pfc)
{
   bool is_solid;

   TerShaderProgramBasic *sh =
      get_shader_program(model, enable_shadow, &is_solid);

   TerShaderProgramModel *sh_model = (TerShaderProgramModel *) sh;

   glUseProgram(sh->prog.program);

   /* We need to create a new projection matrix that matches the far plane
    * selected, otherwise distances in the Z buffer are relative to
    * TER_NEAR_PLANE = 0.0f and TER_FAR_PLANE = 1.0f and not to far_plane.
    */
   glm::mat4 Projection =
      glm::perspective(DEG_TO_RAD(TER_FOV), TER_ASPECT_RATIO,
                       TER_NEAR_PLANE, render_far_plane);

   glm::mat4 *View = (glm::mat4 *) ter_cache_get("matrix/View");
   glm::mat4 *ViewInv = (glm::mat4 *) ter_cache_get("matrix/ViewInv");
   ter_shader_program_basic_load_VP(sh, &Projection, View, ViewInv);

   ter_shader_program_model_load_materials(&sh_model->model,
      model->materials, TER_MODEL_MAX_MATERIALS * model->num_variants);

   TerLight *light = (TerLight *) ter_cache_get("light/light0");
   ter_shader_program_basic_load_light(sh, light);

   ter_shader_program_basic_load_sky_color(sh, &light->diffuse);

   glm::vec4 *clip_plane = (glm::vec4 *) ter_cache_get("clip/clip-plane-0");
   if (clip_plane)
      ter_shader_program_basic_load_clip_plane(sh, *clip_plane);

   if (enable_shadow) {
      TerShaderProgramShadowData *sh_shadow =
         get_shader_program_shadow_data(sh, is_solid);
      int shadow_map_sampler_unit = is_solid ? 0 : 4;
      TerShadowRenderer *sr =
         (TerShadowRenderer *) ter_cache_get("rendering/shadow-renderer");
      glm::mat4 ShadowMapSpaceVP =
         ter_shadow_renderer_get_shadow_map_space_vp(sr);
      glActiveTexture(GL_TEXTURE0 + shadow_map_sampler_unit);
      glBindTexture(GL_TEXTURE_2D, sr->shadow_map->map->depth_texture);
      ter_shader_program_shadow_data_load(
         sh_shadow, &ShadowMapSpaceVP, TER_SHADOW_DISTANCE,
         TER_SHADOW_MAP_SIZE, shadow_pfc, shadow_map_sampler_unit);
   }

   ter_shader_program_model_load_near_far_planes(&sh_model->model,
      TER_NEAR_PLANE, clip_far_plane, render_far_plane);

   if (!is_solid) {
      for (unsigned i = 0; i < model->num_tids; i++) {
         glActiveTexture(GL_TEXTURE0 + i);
         glBindTexture(GL_TEXTURE_2D, model->tids[i]);
      }
      TerShaderProgramModelTex *sh_tex = (TerShaderProgramModelTex *) sh;
      ter_shader_program_model_tex_load_textures(sh_tex, model->num_tids);
   }

   model_bind_vao(model, M4x4_list, num_instances);

   return sh;
}

void
ter_model_render_prepare_for_shadow_map(TerModel *model,
                                        float *M4x4_list,
                                        unsigned num_instances)
{
   /* We expect that we render the first frame without shadows to generate 
    * the VAO.
    */
   assert(model->vao);

   upload_instanced_data_and_bind(model, M4x4_list, num_instances);
   for (int i = 0; i < 5; i++)
      glEnableVertexAttribArray(i);
}


void
ter_model_render_finish(TerModel *model)
{
   model_unbind(model);
}

void
ter_model_render_finish_for_shadow_map(TerModel *model)
{
   for (int i = 0; i < 5; i++)
      glDisableVertexAttribArray(i);
   glBindVertexArray(0);
}

bool
ter_model_is_textured(TerModel *m)
{
   return m->uvs.size() > 0;
}

void
ter_model_add_variant(TerModel *m, TerMaterial *material, unsigned *tids,
                      unsigned count)
{
   assert(m->num_materials == count);
   assert(m->num_variants < TER_MODEL_MAX_VARIANTS);

   for (unsigned i = 0; i < count; i++) {
      m->materials[TER_MODEL_MAX_MATERIALS * m->num_variants + i] = material[i];
      m->tids[TER_MODEL_MAX_TEXTURES * m->num_variants + i] = tids[i];
   }

   m->num_variants++;
}
