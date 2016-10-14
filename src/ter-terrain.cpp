#include "main.h"

TerTerrain *
ter_terrain_new(unsigned width, unsigned depth, float step)
{
   TerTerrain *t = g_new0(TerTerrain, 1);
   t->width = width;
   t->depth = depth;
   t->step = step;
   t->height = g_new0(float, width * depth);
   return t;
}

void
ter_terrain_free(TerTerrain *t)
{
   glDeleteVertexArrays(1, &t->vao);
   glDeleteVertexArrays(1, &t->vertex_buf);
   glDeleteVertexArrays(TER_TERRAIN_NUM_INDEX_BUFFERS, &t->index_buf[0]);

   ter_mesh_free(t->mesh);
   g_free(t->height);
   g_free(t->indices);
   g_free(t);
}

void
ter_terrain_set_height(TerTerrain *t, unsigned w, unsigned d, float h)
{
   TERRAIN(t, w, d) = h;
}

float
ter_terrain_get_height_at(TerTerrain *t, float x, float z)
{
   /* Terrain's Z extends towards -Z, but our vertices need positive numbers */
   z = -z;

   /* Find offsets of the coords into a terrain quad */
   float offx = fmodf(x, t->step);
   float offz = fmodf(z, t->step);

   /* Compute the plane equation for the triangle we are in */
   glm::vec3 p1, p2, p3;
   float A, B, C, D;
   if (offx + offz <= t->step) {
      /* First triangle in the quad */
      p1.x = trunc(x / t->step);
      p1.z = trunc(z / t->step);
      p1.y = TERRAIN(t, (int)p1.x, (int)p1.z);

      p2.x = trunc(x / t->step) + 1;
      p2.z = trunc(z / t->step);
      p2.y = TERRAIN(t, (int)p2.x, (int)p2.z);

      p3.x = trunc(x / t->step);
      p3.z = trunc(z / t->step) + 1;
      p3.y = TERRAIN(t, (int)p3.x, (int)p3.z);
   } else {
      /* Second triangle in the quad */
      p1.x = trunc(x / t->step) + 1;
      p1.z = trunc(z / t->step);
      p1.y = TERRAIN(t, (int)p1.x, (int)p1.z);

      p2.x = trunc(x / t->step);
      p2.z = trunc(z / t->step) + 1;
      p2.y = TERRAIN(t, (int)p2.x, (int)p2.z);

      p3.x = trunc(x / t->step) + 1;
      p3.z = trunc(z / t->step) + 1;
      p3.y = TERRAIN(t, (int)p3.x, (int)p3.z);
   }

   /* Above we compute X,Z coords as vertex indices so we could use TERRAIN()
    * to compute heights at specific vertices, but to apply the plane equation
    * we need to turn the coordinates into world units
    */
   p1.x *= t->step;
   p1.z *= t->step;
   p2.x *= t->step;
   p2.z *= t->step;
   p3.x *= t->step;
   p3.z *= t->step;

   /* FIXME: we probably want to pre-compute plane equations for each
    * triangle in the terrain rather than recomputing them all the time
    */
   A = (p2.y - p1.y) * (p3.z - p1.z) - (p3.y - p1.y) * (p2.z - p1.z);
   B = (p2.z - p1.z) * (p3.x - p1.x) - (p3.z - p1.z) * (p2.x - p1.x);
   C = (p2.x - p1.x) * (p3.y - p1.y) - (p3.x - p1.x) * (p2.y - p1.y);
   D = -(A * p1.x + B * p1.y + C * p1.z);

   /* Use the plane equation to find Y given (X,Z) */
   return (-D - C * z - A * x) / B;
}

void
ter_terrain_set_heights_from_texture(TerTerrain *t, int texture,
                                     float offset, float scale)
{
   TerTextureManager *tex_mgr =
      (TerTextureManager *) ter_cache_get("textures/manager");
   
   SDL_Surface *image = ter_texture_manager_get_image(tex_mgr, texture);
   uint8_t *pixels = (uint8_t *) image->pixels;
   float scale_x = ((float) image->w) / (t->width - 1);
   float scale_z = ((float) image->h) / (t->depth - 1);

   for (int x = 0; x < t->width; x++) {
      for (int z = 0; z < t->depth; z++) {
         int img_x = (int) truncf(x * scale_x);
         int img_y = (int) truncf(z * scale_z);
         float h = pixels[img_y * image->pitch + img_x * 4];

         /* Normalize height to [-1, 1] */ 
         h = h / 127.5 - 1.0f;

         /* Apply scale */
         h *= scale;

         /* Apply height offset */
         h += offset;

         ter_terrain_set_height(t, x, z, h);
      }
   }
}

static glm::vec3
calculate_normal(TerTerrain *t, unsigned x, unsigned z)
{
   if (x == 0)
      x = 1;
   if (z == 0)
      z = 1;
   float hl = TERRAIN(t, x - 1, z);
   float hr = TERRAIN(t, x + 1, z);
   float hd = TERRAIN(t, x, z + 1); /* Terrain expands towards -Z */
   float hu = TERRAIN(t, x, z - 1);
   glm::vec3 n = glm::vec3(hl - hr, 2.0f, hd - hu);
   ter_util_vec3_normalize(&n);
   return n;
}

static void
compute_indices_for_clip_volume(TerTerrain *t, TerClipVolume *clip)
{
   int min_col = MAX(MIN(clip->x0 / t->step, t->width - 2), 0);
   int max_col = MAX(MIN((clip->x1 + t->step) / t->step, t->width - 2), 0);

   /* Z1 is always largest, because our terrain grows towrds -Z, it means that
    * it defines the min_row
    */
   int min_row = MAX(MIN(-clip->z1 / t->step, t->depth - 1), 0);
   int max_row = MAX(MIN(- (clip->z0 - t->step) / t->step, t->depth - 1), 0);

   /* If this happens then the terrain is not visible */
   if (min_col == max_col || min_row == max_row) {
      t->num_indices = 0;
      return;
   }

   unsigned index = 0;
   for (int c = min_col; c <= max_col; c++) {
      for (int r = min_row; r <= max_row; r++) {
         /* If this is not the first strip then we need to produce a degenerate
          * link with the previous strip using the first vertex from this strip
          * and the last vertex from the last before we start recording the
          * new strip. Here is the first vertex of this strip.
          */
         if (c > min_col && r == min_row)
            t->indices[index++] = c * t->depth + r;

         t->indices[index++] = c * t->depth + r;
         t->indices[index++] = (c + 1) * t->depth + r;

         /* Link the next triangle strip using degenerate triangles. For that
          * we need to duplicate the last vertex of this strip and the first
          * vertex of the next.
          */
         if (r == max_row && c < max_col)
            t->indices[index++] = (c + 1) * t->depth + r;
      }
   }

   t->num_indices = index;
}

void
ter_terrain_build_mesh(TerTerrain *t)
{
   /* GL's +Z axis goes towards the camera, so make the terrain's Z coordinates
    * negative so that larger (negative) Z coordinates are more distant.
    */
   t->mesh = ter_mesh_new();

   int vertices_w = t->width;
   int vertices_d = t->depth;

   /* Add each vertex in the grid. We are going to render using an index buffer
    * so we don't need to duplicate vertices, which reduces massively the
    * storage requirements for the vertex data, since terrains have high
    * vertex counts.
    */
   for (int vx = 0; vx < vertices_w; vx++) {
      for (int vz = 0; vz < vertices_d; vz++) {
         float vy = TERRAIN(t, vx, vz);
         glm::vec3 v0 = glm::vec3(vx * t->step, vy, -vz * t->step);
         glm::vec3 n0 = calculate_normal(t, vx, vz);
         ter_mesh_add_vertex(t->mesh, v0.x, v0.y, v0.z, n0.x, n0.y, n0.z);
      }
   }

   /* Build the indices to render the terrain using a single triangle strip
    * (using degenerate triangles) since that yields much better performance
    * than rendering triangles.
    */
   unsigned num_indices =
      (vertices_w - 1) * (vertices_d * 2) + (vertices_w - 2) + (vertices_d - 2);
   t->indices = g_new0(unsigned, num_indices);

   /* Initialize the number of rendering indices so it covers the entire
    * terrain.
    */
   TerClipVolume clip;
   clip.x0 = 0.0f;
   clip.x1 = (t->width - 1) * t->step;
   clip.z1 = 0.0f;
   clip.z0 = -(t->depth - 1) * t->step;
   compute_indices_for_clip_volume(t, &clip);

   assert(num_indices == t->num_indices);
}

static void
terrain_unbind()
{
   glBindTexture(GL_TEXTURE_2D, 0);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   glBindVertexArray(0);
}

static void
terrain_bind_vao(TerTerrain *t)
{
   if (t->vao == 0) {
      TerMesh *mesh = t->mesh;

      unsigned vertex_count = mesh->vertices.size();
      assert(vertex_count == mesh->normals.size());

      /* Interleave attributes for better performance on some platforms. We
       * need a buffer large enough to store 2 vec3 attributes per vertex
       * (position and normal).
       */
      unsigned vertex_byte_size = 2 * sizeof(glm::vec3);
      unsigned vertex_float_size = vertex_byte_size / sizeof(float);
      unsigned bytes = vertex_count * vertex_byte_size;
      uint8_t *vertex_data = g_new(uint8_t, bytes);

      float *vertex_data_f = (float *) vertex_data;
      for (unsigned i = 0; i < vertex_count; i++) {
         memcpy(&vertex_data_f[vertex_float_size * i], &mesh->vertices[i],
                sizeof(glm::vec3));
         memcpy(&vertex_data_f[vertex_float_size * i + 3], &mesh->normals[i],
                sizeof(glm::vec3));
      }

      /* Create vertex buffer and upload data to it */
      glGenBuffers(1, &t->vertex_buf);
      glBindBuffer(GL_ARRAY_BUFFER, t->vertex_buf);
      glBufferData(GL_ARRAY_BUFFER, bytes, vertex_data, GL_STATIC_DRAW);
      g_free(vertex_data);

      /* Create storage for index buffers and upload that to the first */
      t->ibuf_idx = 0;
      glGenBuffers(TER_TERRAIN_NUM_INDEX_BUFFERS, t->index_buf);
      for (unsigned i = 0; i < TER_TERRAIN_NUM_INDEX_BUFFERS; i++) {
         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, t->index_buf[i]);
         glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                      TER_TERRAIN_MAX_IB_BYTES,
                      i == t->ibuf_idx ? t->indices : NULL,
                      GL_DYNAMIC_DRAW);
      }
      t->ibuf_used = t->num_indices;

      glGenVertexArrays(1, &t->vao);
      glBindVertexArray(t->vao);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, t->index_buf[t->ibuf_idx]);

      glEnableVertexAttribArray(0);
      glVertexAttribPointer(
         0,                  // Attribute index
         3,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         vertex_byte_size,   // stride
         (void*)0            // array buffer offset
      );

      glEnableVertexAttribArray(1);
      glVertexAttribPointer(
         1,                  // Attribute index
         3,                  // size
         GL_FLOAT,           // type
         GL_FALSE,           // normalized?
         vertex_byte_size,   // stride
         (void*)sizeof(glm::vec3) // array buffer offset
      );

      ter_dbg(LOG_VBO,
              "TERRAIN: VBO: INFO: Uploaded %u bytes (%u KB) "
              "for %u vertices (%u bytes/vertex)\n",
              bytes, bytes / 1024, vertex_count, vertex_byte_size);

      unsigned index_bytes = TER_TERRAIN_MAX_IB_BYTES;
      unsigned num_indices = TER_TERRAIN_MAX_IB_BYTES / sizeof(int);
      ter_dbg(LOG_VBO,
              "TERRAIN: VBO: INFO: Uploaded %u bytes (%u KB) "
              "for %u indices (%u bytes/index)\n",
              index_bytes, index_bytes / 1024, num_indices, sizeof(int));
   } else {
      glBindVertexArray(t->vao);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);

      /* Always bind the current index buffer explicitly, it seems as if binding
       * the VAO doesn't reliably bind the first index buffer when ibuf_idx = 0.
       * Otherwise we should  only need to bind if ibuf_idx > 0, but this does not
       * work. Maybe intel drivers are not storing the index buffer binding in
       * the VAO state?
       */
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, t->index_buf[t->ibuf_idx]);
   }
}

static void
terrain_prepare(TerTerrain *t, bool enable_shadows)
{
   TerShaderProgramTerrain *sh = (TerShaderProgramTerrain *)
      (enable_shadows ? ter_cache_get("program/terrain-shadow") :
                        ter_cache_get("program/terrain"));
   glUseProgram(sh->basic.prog.program);

   glm::mat4 *Projection = (glm::mat4 *) ter_cache_get("matrix/Projection");
   glm::mat4 *View = (glm::mat4 *) ter_cache_get("matrix/View");
   glm::mat4 *ViewInv = (glm::mat4 *) ter_cache_get("matrix/ViewInv");
   glm::mat4 Model = glm::mat4(1.0);
   ter_shader_program_basic_load_MVP(&sh->basic,
                                     Projection, View, ViewInv, &Model);

   TerLight *light = (TerLight *) ter_cache_get("light/light0");
   ter_shader_program_basic_load_light(&sh->basic, light);
   ter_shader_program_basic_load_sky_color(&sh->basic, &light->diffuse);

   ter_shader_program_basic_load_material(&sh->basic, &t->material);

   TerTextureManager *tex_mgr =
      (TerTextureManager *) ter_cache_get("textures/manager");
   unsigned tid =
      ter_texture_manager_get_tid(tex_mgr, TER_TEX_TERRAIN_SURFACE_01);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, tid);
   ter_shader_program_terrain_load_sampler(sh, 0, 4.0);

   glm::vec4 *clip_plane = (glm::vec4 *) ter_cache_get("clip/clip-plane-0");
   if (clip_plane)
      ter_shader_program_basic_load_clip_plane(&sh->basic, *clip_plane);

   if (enable_shadows) {
      TerShadowRenderer *sr =
         (TerShadowRenderer *) ter_cache_get("rendering/shadow-renderer");
      glm::mat4 ShadowMapSpaceVP =
         ter_shadow_renderer_get_shadow_map_space_vp(sr);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, sr->shadow_map->map->depth_texture);
      ter_shader_program_shadow_data_load(
         &sh->shadow, &ShadowMapSpaceVP, TER_SHADOW_DISTANCE,
         TER_SHADOW_MAP_SIZE, TER_SHADOW_PFC, 1);
   }

   terrain_bind_vao(t);
}

static inline void
terrain_finish()
{
   terrain_unbind();
}

void
ter_terrain_compute_clipped_indices(TerTerrain *t, TerClipVolume *clip,
                                    unsigned *count, size_t *offset)
{
   /* Our terrian indices cover terrain rendering by columns, so compute
    * how many columns in the vertex grid are to be cliped at the start
    * and the end of the terrain based on the x0/x1 dimensions of the clip
    * volume
    */
   int min_col = MAX(MIN(clip->x0 / t->step, t->width), 0);
   int max_col = MAX(MIN((clip->x1 + t->step) / t->step, t->width - 2), 0);

   /* For each column in the terrain, we render 2 * depth vertices, plus
    * 2 extra vertices to produce a degenerate triangle so we can join the
    * strip for the next column.
    */
   int indices_per_col = 2 * t->depth + 2;

   /* Compute how many indices we need to skip at the start of the index buffer
    * to start rendering at "min_col" and how many indices we can skip at
    * the tail of the buffer index to stop rendering at "max_col". Also, notice
    * that the last column we render has index "width - 2" and that for that
    * column we don't have those 2 extra indices to produce a degenerate (since
    * that is the last strip to render).
    */
   int clipped_indices_start = min_col * indices_per_col;
   int clipped_indices_end =
      MAX(0, ((t->width - 2) - max_col) * indices_per_col - 2);

   *offset = min_col * indices_per_col * sizeof(int);
   *count = t->num_indices - clipped_indices_start - clipped_indices_end;
}

size_t
ter_terrain_update_index_buffer_for_clip_volume(TerTerrain *t,
                                                TerClipVolume *clip)
{
   compute_indices_for_clip_volume(t, clip);

   /* Upload the new index data */
   size_t buffer_offset = 0;

   /* See if we have enough room in the current index buffer for the indices
    * we want to upload. If not, bind the next buffer and upload there.
    */
   unsigned ibuf_available = TER_TERRAIN_MAX_IB_INDICES - t->ibuf_used;
   if (t->num_indices > ibuf_available) {
      t->ibuf_idx++;
      if (t->ibuf_idx >= TER_TERRAIN_NUM_INDEX_BUFFERS)
         t->ibuf_idx = 0;
      t->ibuf_used = t->num_indices;
   } else {
      buffer_offset = t->ibuf_used * sizeof(unsigned);
      t->ibuf_used += t->num_indices;
   }

   unsigned index_bytes = t->num_indices * sizeof(unsigned);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, t->index_buf[t->ibuf_idx]);
   glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, buffer_offset,
                   index_bytes, t->indices);

   ter_dbg(LOG_VBO,
           "TERRAIN: VBO: INFO: Uploaded %u bytes (%u KB) "
           "for %u indices (%u bytes/index) (buf=%u, offset=%u)\n",
           index_bytes, index_bytes / 1024, t->num_indices, sizeof(int),
           t->ibuf_idx, buffer_offset);

   return buffer_offset;
}

static inline size_t
get_current_ib_offset(TerTerrain *t)
{
   /* After uploading data to the index buffer we have moved ibuf_used
    * exactly by num_indices so ibuf_used points right after the last
    * slot used. To compute where the current index buffer starts, we
    * have to remove the number of indices currently active.
    */
   return (t->ibuf_used - t->num_indices) * sizeof(unsigned);
}

/*
 * Notice that this expects that the index buffer has been properly
 * updated with the current active indices. To do that, callers of this
 * function should've called ter_terrain_update_index_buffer_for_clip_volume()
 * prior to calling this, which updates the index buffer with the indices
 * relevant to the clipping region passed as parameter. This will simply
 * render t->num_vertices at the currently active index buffer offset
 * where ter_terrain_update_index_buffer_for_clip_volume() uploaded index data.
 */
void
ter_terrain_render(TerTerrain *t, bool enable_shadows)
{
   terrain_prepare(t, enable_shadows);
   glDrawElements(GL_TRIANGLE_STRIP, t->num_indices,
                  GL_UNSIGNED_INT, (void *) get_current_ib_offset(t));
   terrain_finish();
}

float
ter_terrain_get_width(TerTerrain *t)
{
   return t->step * (t->width - 1);
}

float
ter_terrain_get_depth(TerTerrain *t)
{
   return t->step * (t->depth - 1);
}
