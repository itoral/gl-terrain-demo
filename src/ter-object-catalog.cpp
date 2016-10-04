#include "ter-object-catalog.h"
#include "ter-model.h"
#include "ter-cache.h"

static TerObject *
create_object(const char *id, float x, float y, float z,
              float sx, float sy, float sz)
{
   TerModel *m = (TerModel *) ter_cache_get(id);
   TerObject *o = ter_object_new(m, x, y, z);
   o->scale = glm::vec3(sx, sy, sz);
   o->pos -= m->center * o->scale; /* Center the object in the given position */
   o->pos.y += ter_object_get_height(o) / 2.0f; /* Make Y == object bottom */

   ter_object_update_box(o);

   return o;
}

TerObject *
ter_object_catalog_new_tree1(float x, float y, float z)
{
   return create_object("model/tree-01", x, y, z, 0.5f, 0.5f, 0.5f);
}

TerObject *
ter_object_catalog_new_tree1_scale(float x, float y, float z, float s)
{
   return create_object("model/tree-01", x, y, z,
                        0.5f * s, 0.5f * s, 0.5f * s);
}

TerObject *
ter_object_catalog_new_tree2(float x, float y, float z)
{
   return create_object("model/tree-02", x, y, z, 0.007f, 0.007f, 0.007f);
}

TerObject *
ter_object_catalog_new_tree2_scale(float x, float y, float z, float s)
{
   return create_object("model/tree-02", x, y, z,
                        0.007f * s, 0.007f * s, 0.007f * s);
}

TerObject *
ter_object_catalog_new_tree3_scale(float x, float y, float z, float s)
{
   return create_object("model/tree-03", x, y, z,
                        0.66f * s, 0.66f * s, 0.66f * s);
}

TerObject *
ter_object_catalog_new_rock1(float x, float y, float z)
{
   return create_object("model/rock-01", x, y, z, 1.0f, 1.0f, 1.0f);
}

TerObject *
ter_object_catalog_new_rock1_scale(float x, float y, float z, float s)
{
   return create_object("model/rock-01", x, y - 0.25, z,
                        1.0f * s, 1.0f * s, 1.0f * s);
}

TerObject *
ter_object_catalog_new_rock2(float x, float y, float z)
{
   return create_object("model/rock-02", x, y, z, 0.5f, 0.5f, 0.5f);
}

TerObject *
ter_object_catalog_new_rock2_scale(float x, float y, float z, float s)
{
   TerObject *obj =
      create_object("model/rock-02", x, y, z, 0.5f * s, 0.5f * s, 0.5f * s);
   obj->rot = glm::vec3(90.0f, 0.0f, 0.0f);
   return obj;   
}

TerObject *
ter_object_catalog_new_grass1(float x, float y, float z)
{
   TerObject *o = create_object("model/grass-01", x, y, z, 0.5f, 0.5f, 0.5f);
   o->cast_shadow = false;
   return o;
}

TerObject *
ter_object_catalog_new_grass1_scale(float x, float y, float z, float s)
{
   TerObject *o = create_object("model/grass-01", x, y, z,
                                0.5f * s, 0.5f * s, 0.5f * s);
   o->cast_shadow = false;
   o->can_collide = false;
   return o;
}

TerObject *
ter_object_catalog_new_grass2_scale(float x, float y, float z, float s)
{
   TerObject *o = create_object("model/grass-02", x, y, z, s, s, s);
   o->cast_shadow = false;
   o->can_collide = false;
   return o;
}

