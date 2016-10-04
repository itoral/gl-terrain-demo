#ifndef __DRV_MESH_H__
#define __DRV_MESH_H__

#include <vector>

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

typedef struct {
   std::vector<glm::vec3> vertices;
   std::vector<glm::vec3> normals;
} TerMesh;

TerMesh *ter_mesh_new();
void ter_mesh_free(TerMesh *mesh);
void ter_mesh_add_vertex(TerMesh *mesh,
                         float x, float y, float z,
                         float nx, float ny, float nz);
#endif
