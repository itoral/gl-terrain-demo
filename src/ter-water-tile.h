#ifndef __TER_WATER_TILE_H__
#define __TER_WATER_TILE_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>

#include "ter-render-texture.h"

typedef struct {
   float x0, z0, x1, z1, h;

   float distortion;
   float wave_factor;
   float wave_speed;
   float reflection_factor;

   glm::vec3 *vertices;
   float tile_size;
   int num_vertices;

   unsigned vao;
   unsigned vertex_buf;

   TerRenderTexture *reflection;
   TerRenderTexture *refraction;
   unsigned dudv_tex;
   unsigned normal_tex;
} TerWaterTile;

TerWaterTile *ter_water_tile_new(float x0, float z0, float x1, float z1,
                                 float h, float water_tile_size,
                                 unsigned dudv_tex, unsigned normal_tex);

void ter_water_tile_free(TerWaterTile *t);

void ter_water_tile_update(TerWaterTile *t);

void ter_water_tile_render(TerWaterTile *t);

#endif
