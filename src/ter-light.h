#ifndef __TER_LIGHT_H__
#define __TER_LIGHT_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>

typedef enum {
   TER_LIGHT_TYPE_DIRECTIONAL,
   TER_LIGHT_TYPE_POSITIONAL,
} TerLightType;

typedef struct {
   TerLightType type;
   glm::vec4 origin;
   glm::vec3 rot;
   glm::vec3 rot_speed;
   float attenuation;
   glm::vec3 diffuse;
   glm::vec3 ambient;
   glm::vec3 specular;
} TerLight;

TerLight *ter_light_new_directional(glm::vec3 dir,
                                    glm::vec3 diffuse,
                                    glm::vec3 ambient,
                                    glm::vec3 specular);

TerLight *ter_light_new_positional(glm::vec3 pos,
                                   float attenuation,
                                   glm::vec3 diffuse,
                                   glm::vec3 ambient,
                                   glm::vec3 specular);

void ter_light_free(TerLight *light);

void ter_light_update(TerLight *light);
glm::vec4 ter_light_get_world_position(TerLight *light);
glm::mat4 ter_light_get_view_matrix(TerLight *light);

#endif
