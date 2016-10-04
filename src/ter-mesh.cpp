#include "ter-mesh.h"

#include <glib.h>

TerMesh *
ter_mesh_new()
{
   TerMesh *mesh = g_new0(TerMesh, 1);
   mesh->vertices = std::vector<glm::vec3>();
   mesh->normals = std::vector<glm::vec3>();
   return mesh;
}

void
ter_mesh_free(TerMesh *mesh)
{
   mesh->vertices.clear();
   std::vector<glm::vec3>(mesh->vertices).swap(mesh->vertices);
   mesh->normals.clear();
   std::vector<glm::vec3>(mesh->normals).swap(mesh->normals);
   g_free(mesh);
}

void
ter_mesh_add_vertex(TerMesh *mesh,
                    float x, float y, float z,
                    float nx, float ny, float nz)
{
   glm::vec3 v = glm::vec3(x, y, z);
   glm::vec3 n = glm::vec3(nx, ny, nz);
   mesh->vertices.push_back(v);
   mesh->normals.push_back(n);
}

