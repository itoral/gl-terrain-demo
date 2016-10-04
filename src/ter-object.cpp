#include "ter-object.h"

#include <glib.h>

TerObject *
ter_object_new(TerModel *model, float x, float y, float z)

{
   TerObject *o = g_new0(TerObject, 1);
   o->model = model;
   o->pos = glm::vec3(x, y, z);
   o->scale = glm::vec3(1.0, 1.0, 1.0);
   o->cast_shadow = true;
   o->can_collide = true;
   return o;
}

void
ter_object_free(TerObject *o)
{
   g_free(o);
}

void
ter_object_render(TerObject *o, bool enable_shadow)
{
   ter_model_render(o->model, o->pos, o->rot, o->scale, enable_shadow);
}

bool
ter_object_is_rotated(TerObject *o)
{
   return o->rot.x || o->rot.y || o->rot.z;
}

glm::mat4
ter_object_get_model_matrix(TerObject *o)
{
   bool is_rotated = ter_object_is_rotated(o);

   /* 1. Center the model around <0,0,0> if we need to rotate
    * 2. Rotate (Z, Y, X)
    * 3. Un-center the model again
    *
    * Up to this point we completely ignored the scale, we just rotated it
    * around its center. Now, we need to scale the object and then move it
    * because when we create objects via de object catalog we already adjust
    * the position of the object taking into account the scale factor and the
    * center offset:
    *
    * 4. Scale
    * 5. Move
    */

   glm::mat4 Model = glm::mat4(1.0);
   Model = glm::translate(Model, o->pos);
   Model = glm::scale(Model, o->scale);
   if (is_rotated)
      Model = glm::translate(Model, o->model->center);
   if (o->rot.x)
     Model = glm::rotate(Model, DEG_TO_RAD(o->rot.x), glm::vec3(1, 0, 0));
   if (o->rot.y)
      Model = glm::rotate(Model, DEG_TO_RAD(o->rot.y), glm::vec3(0, 1, 0));
   if (o->rot.z)
      Model = glm::rotate(Model, DEG_TO_RAD(o->rot.z), glm::vec3(0, 0, 1));
   if (is_rotated)
      Model = glm::translate(Model, -o->model->center);
   return Model;
}

float
ter_object_get_height(TerObject *o)
{
   return o->model->h * o->scale.y;
}

float
ter_object_get_width(TerObject *o)
{
   return o->model->w * o->scale.x;
}

float
ter_object_get_depth(TerObject *o)
{
   return o->model->d * o->scale.z;
}

glm::vec3
ter_object_get_position(TerObject *o)
{
   glm::vec3 pos = o->pos;
   pos += o->model->center * o->scale; /* Center the object in the given position */
   pos.y -= ter_object_get_height(o) / 2.0f; /* Make Y = object bottom */
   return pos;
}

/*
 * Updates the bounding box for the object considering its transforms
 * (position, scale and rotation). This works by applying the transforms
 * of the object to its original bounding box and producing an axis-aligned
 * box from the result.
 *
 * FIXME: for static objects we can do better, we can apply the transforms
 * to all the vertices in the model and compute the new axis aligned
 * bounding box. This would provide a tighter box for rotated objects.
 */
void
ter_object_update_box(TerObject *o)
{
   o->box.w = ter_object_get_width(o) / 2.0f;
   o->box.d = ter_object_get_depth(o) / 2.0f;
   o->box.h = ter_object_get_height(o) / 2.0f;
   o->box.center = ter_object_get_position(o);
   o->box.center.y += o->box.h;

   if (ter_object_is_rotated(o)) {
      glm::mat4 transform = ter_object_get_model_matrix_for_box(o);
      ter_box_transform(&o->box, &transform);
   }
}

/*
 * Returns the axis-aligned bounding box for the object. Notice that the
 * box must be updated if the object transforms have changed.
 */
TerBox *
ter_object_get_box(TerObject *o)
{
   return &o->box;
}

bool
ter_object_collision(TerObject *o, TerBox *box)
{
   TerBox *o_box = ter_object_get_box(o);
   return ter_box_collision(box, o_box);
}

glm::mat4
ter_object_get_model_matrix_for_box(TerObject *o)
{
   /* The bounding box is centered in world units (it does not account for the
    * model center offset) and its coordinates already include the scaling
    * factor of the object. This basically means that we only care about the
    * rotation aspect.
    *
    * Thus, get the world unit coordinates of its center (the obect position
    * plus scaled center offset) and rotate around that point.
    */

   glm::mat4 Model(1.0f);
   Model = glm::translate(Model, o->pos + o->model->center * o->scale);
   if (o->rot.x)
      Model = glm::rotate(Model, DEG_TO_RAD(o->rot.x), glm::vec3(1, 0, 0));
   if (o->rot.y)
      Model = glm::rotate(Model, DEG_TO_RAD(o->rot.y), glm::vec3(0, 1, 0));
   if (o->rot.z)
      Model = glm::rotate(Model, DEG_TO_RAD(o->rot.z), glm::vec3(0, 0, 1));
   Model = glm::translate(Model, -o->pos - o->model->center * o->scale);

   return Model;
}
