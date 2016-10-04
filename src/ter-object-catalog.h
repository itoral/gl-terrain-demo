#ifndef __TER_OBJECT_CATALOG_H__
#define __TER_OBJECT_CATALOG_H__

#include "ter-object.h"

TerObject *ter_object_catalog_new_tree1(float x, float y, float z);
TerObject *ter_object_catalog_new_tree1_scale(float x, float y, float z, float s);

TerObject *ter_object_catalog_new_tree2(float x, float y, float z);
TerObject *ter_object_catalog_new_tree2_scale(float x, float y, float z, float s);

TerObject *ter_object_catalog_new_tree3_scale(float x, float y, float z, float s);

TerObject *ter_object_catalog_new_rock1(float x, float y, float z);
TerObject *ter_object_catalog_new_rock1_scale(float x, float y, float z, float s);

TerObject *ter_object_catalog_new_rock2(float x, float y, float z);
TerObject *ter_object_catalog_new_rock2_scale(float x, float y, float z, float s);

TerObject *ter_object_catalog_new_grass1(float x, float y, float z);
TerObject *ter_object_catalog_new_grass1_scale(float x, float y, float z, float s);

TerObject *ter_object_catalog_new_grass2_scale(float x, float y, float z, float s);

#endif
