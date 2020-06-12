#ifndef __TER_TILE_H__
#define __TER_TILE_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>

typedef struct {
   unsigned width, height;

   glm::vec2 vertices[6];
   glm::vec2 uvs[6];

   glm::vec3 pos;
   glm::vec3 rot;
   glm::vec3 scale;
   unsigned texture;
   unsigned sampler;

   unsigned vao;
   unsigned vertex_buf;
   unsigned uv_buffer;
} TerTile;

TerTile *ter_tile_new(unsigned width, unsigned height, float x, float y,
                      unsigned texture, unsigned sampler);
void ter_tile_render(TerTile *tile);

#endif
