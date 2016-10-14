#include "ter-box.h"
#include "ter-util.h"

bool ter_box_is_inside(TerBox *box, glm::vec3 &p) __attribute__ ((unused));
bool
ter_box_is_inside(TerBox *box, glm::vec3 &p)
{
   float xmin = box->center.x - box->w;
   float xmax = box->center.x + box->w;
   float ymin = box->center.y - box->h;
   float ymax = box->center.y + box->h;
   float zmin = box->center.z - box->d;
   float zmax = box->center.z + box->d;

   return p.x >= xmin && p.x <= xmax &&
          p.y >= ymin && p.y <= ymax &&
          p.z >= zmin && p.z <= zmax;
}

bool
ter_box_collision(TerBox *box1, TerBox *box2)
{
   float xmin1 = box1->center.x - box1->w;
   float xmax1 = box1->center.x + box1->w;
   float xmin2 = box2->center.x - box2->w;
   float xmax2 = box2->center.x + box2->w;

   if (xmax1 < xmin2 || xmax2 < xmin1)
      return false;

   float ymin1 = box1->center.y - box1->h;
   float ymax1 = box1->center.y + box1->h;
   float ymin2 = box2->center.y - box2->h;
   float ymax2 = box2->center.y + box2->h;

   if (ymax1 < ymin2 || ymax2 < ymin1)
      return false;

   float zmin1 = box1->center.z - box1->d;
   float zmax1 = box1->center.z + box1->d;
   float zmin2 = box2->center.z - box2->d;
   float zmax2 = box2->center.z + box2->d;

   if (zmax1 < zmin2 || zmax2 < zmin1)
      return false;

   return true;
}

void
ter_box_transform(TerBox *box, glm::mat4 *transform)
{
   glm::vec3 vertices[8];

   /* Transform the box vertices */
   int n = 0;
   for (float w = -box->w; w <= box->w; w += 2.0f * box->w) {
      for (float h = -box->h; h <= box->h; h += 2.0f * box->h) {
         for (float d = -box->d; d <= box->d; d += 2.0f * box->d) {
            glm::vec4 v = vec4(box->center + glm::vec3(w, h, d), 1.0f);
            v = (*transform) * v;
            vertices[n++] = vec3(v);
         }
      }
   }

   /* Compute transformed box position and dimensions */
   float minX = vertices[0].x;
   float maxX = vertices[0].x;
   float minY = vertices[0].y;
   float maxY = vertices[0].y;
   float minZ = vertices[0].z;
   float maxZ = vertices[0].z;
   for (int i = 1; i < 8; i++) {
      if (vertices[i].x > maxX)
         maxX = vertices[i].x;
      else if (vertices[i].x < minX)
         minX = vertices[i].x;

      if (vertices[i].y > maxY)
         maxY = vertices[i].y;
      else if (vertices[i].y < minY)
         minY = vertices[i].y;

      if (vertices[i].z > maxZ)
         maxZ = vertices[i].z;
      else if (vertices[i].z < minZ)
         minZ = vertices[i].z;
   }

   box->center = glm::vec3((maxX + minX) / 2.0f,
                           (maxY + minY) / 2.0f,
                           (maxZ + minZ) / 2.0f);
   box->w = (maxX - minX) / 2.0f;
   box->h = (maxY - minY) / 2.0f;
   box->d = (maxZ - minZ) / 2.0f;
}

