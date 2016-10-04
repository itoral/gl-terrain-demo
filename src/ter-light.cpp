#include "main.h"

static void
init_light(TerLight *l, float attenuation, glm::vec3 diffuse,
           glm::vec3 ambient, glm::vec3 specular)
{
   l->attenuation = attenuation;
   l->diffuse = diffuse;
   l->ambient = ambient;
   l->specular = specular;
}

TerLight *
ter_light_new_directional(glm::vec3 dir,
                          glm::vec3 diffuse,
                          glm::vec3 ambient,
                          glm::vec3 specular)
{
   glm::vec4 _pos(dir, 0.0f); /* .w = 0 => directional */
   TerLight *l = g_new0(TerLight, 1);
   init_light(l, 1.0f /* No att. */, diffuse, ambient, specular);
   l->type = TER_LIGHT_TYPE_DIRECTIONAL;
   l->origin = _pos;
   return l;
}

TerLight *
ter_light_new_positional(glm::vec3 pos,
                         float attenuation,
                         glm::vec3 diffuse,
                         glm::vec3 ambient,
                         glm::vec3 specular)
{
   glm::vec4 _pos(pos, 1.0f); /* .w = 1 => positional */
   TerLight *l = g_new0(TerLight, 1);
   init_light(l, attenuation, diffuse, ambient, specular);
   l->type = TER_LIGHT_TYPE_POSITIONAL;
   l->origin = _pos;
   return l;
}

void
ter_light_free(TerLight *light)
{
   g_free(light);
}

void
ter_light_update(TerLight *light)
{
   light->rot += light->rot_speed;
}

glm::vec4
ter_light_get_world_position(TerLight *light)
{
   glm::mat4 Model = glm::mat4(1.0);
   Model = glm::rotate(Model, DEG_TO_RAD(light->rot.x), glm::vec3(1, 0, 0));
   Model = glm::rotate(Model, DEG_TO_RAD(light->rot.y), glm::vec3(0, 1, 0));
   Model = glm::rotate(Model, DEG_TO_RAD(light->rot.z), glm::vec3(0, 0, 1));
   glm::vec3 origin =
      glm::vec3(light->origin.x, light->origin.y, light->origin.z);
   Model = glm::translate(Model, origin);
   glm::vec4 pos = Model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
   return glm::vec4(pos.x, pos.y, pos.z, light->origin.w);
}

glm::mat4
ter_light_get_view_matrix(TerLight *light)
{
   glm::mat4 mat(1.0);
   float rx = DEG_TO_RAD(light->rot.x);
   float ry = DEG_TO_RAD(light->rot.y);
   float rz = DEG_TO_RAD(light->rot.z);
   mat = glm::rotate(mat, -rx, glm::vec3(1, 0, 0));
   mat = glm::rotate(mat, -ry, glm::vec3(0, 1, 0));
   mat = glm::rotate(mat, -rz, glm::vec3(0, 0, 1));
   glm::vec3 origin(light->origin.x, light->origin.y, light->origin.z);
   mat = glm::translate(mat, -origin);
   return mat;
}

