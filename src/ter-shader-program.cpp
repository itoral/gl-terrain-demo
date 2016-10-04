#include "ter-shader-program.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <glib.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>

static char *
read_shader_file(const char *file)
{
   std::string shaderCode;
   std::ifstream shaderStream(file, std::ios::in);
   if(shaderStream.is_open()) {
      std::string Line = "";
      while(getline(shaderStream, Line))
         shaderCode += Line + "\n";
      shaderStream.close();
   }

   const char *sourcePtr = shaderCode.c_str();
   int sourceLength = strlen(sourcePtr);
   char *source = (char *) malloc(sourceLength + 1);
   memcpy(source, sourcePtr, sourceLength);
   source[sourceLength] = '\0';

   return source;
}

static void
compile_shader(GLuint shaderID, const char *file)
{
   GLint result;
   int infoLogLength;

   ter_dbg(LOG_SHADER, "SHADER: INFO: Compiling shader %d: %s\n",
           shaderID, file);

   char *source = read_shader_file(file); 
   glShaderSource(shaderID, 1, (const char **)&source , NULL);
   glCompileShader(shaderID);
   free(source);

   glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
   glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
   if (result == GL_FALSE) {
      ter_dbg(LOG_SHADER, "SHADER: ERROR: ");
      if (infoLogLength > 0){
         std::vector<char> errorMessage(infoLogLength + 1);
         glGetShaderInfoLog(shaderID, infoLogLength, NULL, &errorMessage[0]);
         ter_dbg(LOG_SHADER, "%s\n", &errorMessage[0]);
      }
      printf("ERROR: failed to compile shader '%s'\n", file);
      exit(1);
   }
}

static unsigned
link_program(GLuint vertexShaderID, GLuint fragmentShaderID)
{
   GLuint programID = glCreateProgram();
   glAttachShader(programID, vertexShaderID);
   glAttachShader(programID, fragmentShaderID);
   glLinkProgram(programID);

   GLint result;
   int infoLogLength;
   glGetProgramiv(programID, GL_LINK_STATUS, &result);
   glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
   if (result == GL_FALSE) {
      ter_dbg(LOG_SHADER, "SHADER: ERROR: ");
      if (infoLogLength > 0) {
         std::vector<char> errorMessage(infoLogLength + 1);
         glGetProgramInfoLog(programID, infoLogLength, NULL, &errorMessage[0]);
         ter_dbg(LOG_SHADER, "%s\n", &errorMessage[0]);
      }
      printf("ERROR: failed to link shader program\n");
      exit(1);
   }

   ter_dbg(LOG_SHADER, "SHADER: INFO: Linked shader program %d: "
           "vs(%d) + fs(%d)\n", programID, vertexShaderID, fragmentShaderID);

   glDeleteShader(vertexShaderID);
   glDeleteShader(fragmentShaderID);

   return programID;
}

unsigned
build_shader_program(const char *vertexFile, const char *fragmentFile)
{
   GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
   compile_shader(vertexShaderID, vertexFile);

   GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
   compile_shader(fragmentShaderID, fragmentFile);

   return link_program(vertexShaderID, fragmentShaderID);
}

static void
init_program(TerShaderProgram *p, unsigned programID)
{
   p->program = programID;
}

void
ter_shader_program_free(TerShaderProgram *sh)
{
   glDeleteProgram(sh->program);
   g_free(sh);
}

TerShaderProgramTile *
ter_shader_program_tile_new()
{
   unsigned programID = build_shader_program("../shaders/tile.vert",
                                             "../shaders/tile.frag");

   TerShaderProgramTile *p = g_new0(TerShaderProgramTile, 1);
   init_program(&p->prog, programID);

   p->projection_loc = glGetUniformLocation(programID, "Projection");
   p->model_loc = glGetUniformLocation(programID, "Model");
   p->texture_loc = glGetUniformLocation(programID, "Tex");

   return p;
}

void
ter_shader_program_tile_load_MVP(TerShaderProgramTile *p,
                                 glm::mat4 *projection, glm::mat4 *model)
{
   glUniformMatrix4fv(p->projection_loc, 1, GL_FALSE, &(*projection)[0][0]);
   glUniformMatrix4fv(p->model_loc, 1, GL_FALSE, &(*model)[0][0]);
}

void
ter_shader_program_tile_load_texture(TerShaderProgramTile *p, unsigned unit)
{
   glUniform1i(p->texture_loc, unit);
}

static void
init_basic(TerShaderProgramBasic *p, unsigned programID)
{
   init_program(&p->prog, programID);

   p->projection_loc = glGetUniformLocation(programID, "Projection");
   p->view_loc = glGetUniformLocation(programID, "View");
   p->view_inv_loc = glGetUniformLocation(programID, "ViewInv");
   p->model_loc = glGetUniformLocation(programID, "Model");
   p->model_it_loc = glGetUniformLocation(programID, "ModelInvTransp");

   p->light_pos_loc = glGetUniformLocation(programID, "LightPosition");
   p->light_attenuation_loc = glGetUniformLocation(programID, "LightAttenuation");
   p->light_diffuse_loc = glGetUniformLocation(programID, "LightDiffuse");
   p->light_ambient_loc = glGetUniformLocation(programID, "LightAmbient");
   p->light_specular_loc = glGetUniformLocation(programID, "LightSpecular");

   p->material_diffuse_loc = glGetUniformLocation(programID, "MaterialDiffuse");
   p->material_ambient_loc = glGetUniformLocation(programID, "MaterialAmbient");
   p->material_specular_loc = glGetUniformLocation(programID, "MaterialSpecular");
   p->material_shininess_loc = glGetUniformLocation(programID, "MaterialShininess");

   p->clip_plane_loc = glGetUniformLocation(programID, "ClipPlane");
   p->sky_color_loc = glGetUniformLocation(programID, "SkyColor");
}

static void
load_MVP(TerShaderProgramBasic *p,
         const glm::mat4 *projection,
         const glm::mat4 *view,
         const glm::mat4 *view_inv,
         const glm::mat4 *model)
{
   glUniformMatrix4fv(p->projection_loc, 1, GL_FALSE, &(*projection)[0][0]);
   glUniformMatrix4fv(p->view_loc, 1, GL_FALSE, &(*view)[0][0]);
   glUniformMatrix4fv(p->model_loc, 1, GL_FALSE, &(*model)[0][0]);

   glm::mat3 model_it = glm::transpose(glm::inverse(glm::mat3(*model)));
   glUniformMatrix3fv(p->model_it_loc, 1, GL_FALSE, &model_it[0][0]);

   glUniformMatrix4fv(p->view_inv_loc, 1, GL_FALSE, &(*view_inv)[0][0]);
}

void
ter_shader_program_basic_load_MVP(TerShaderProgramBasic *p,
                                  const glm::mat4 *projection,
                                  const glm::mat4 *view,
                                  const glm::mat4 *view_inv,
                                  const glm::mat4 *model)
{
   load_MVP(p, projection, view, view_inv, model);
}

void
ter_shader_program_basic_load_VP(TerShaderProgramBasic *p,
                                 const glm::mat4 *projection,
                                 const glm::mat4 *view,
                                 const glm::mat4 *view_inv)
{
   glUniformMatrix4fv(p->projection_loc, 1, GL_FALSE, &(*projection)[0][0]);
   glUniformMatrix4fv(p->view_loc, 1, GL_FALSE, &(*view)[0][0]);
   glUniformMatrix4fv(p->view_inv_loc, 1, GL_FALSE, &(*view_inv)[0][0]);
}

void
ter_shader_program_basic_load_M(TerShaderProgramBasic *p,
                                const glm::mat4 *model)
{
   glUniformMatrix4fv(p->model_loc, 1, GL_FALSE, &(*model)[0][0]);
   glm::mat3 model_it = glm::transpose(glm::inverse(glm::mat3(*model)));
   glUniformMatrix3fv(p->model_it_loc, 1, GL_FALSE, &model_it[0][0]);
}

static void
load_light(TerShaderProgramBasic *p, TerLight *light)
{
   glm::vec4 pos = ter_light_get_world_position(light);
   glUniform4f(p->light_pos_loc, pos.x, pos.y, pos.z, pos.w);
   glUniform1f(p->light_attenuation_loc, light->attenuation);
   glUniform3f(p->light_diffuse_loc,
               light->diffuse.r, light->diffuse.g, light->diffuse.b);
   glUniform3f(p->light_ambient_loc,
               light->ambient.r, light->ambient.g, light->ambient.b);
   glUniform3f(p->light_specular_loc,
               light->specular.r, light->specular.g, light->specular.b);
}

void
ter_shader_program_basic_load_light(TerShaderProgramBasic *p,
                                    TerLight *light)
{
   load_light(p, light);
}

void
ter_shader_program_basic_load_sky_color(TerShaderProgramBasic *p,
                                        glm::vec3 *color)
{
   glUniform3f(p->sky_color_loc, color->x, color->y, color->z);
}

static void
load_material(TerShaderProgramBasic *p, TerMaterial *material)
{
   glUniform3f(p->material_diffuse_loc,
               material->diffuse.r, material->diffuse.g, material->diffuse.b);
   glUniform3f(p->material_ambient_loc,
               material->ambient.r, material->ambient.g, material->ambient.b);
   glUniform3f(p->material_specular_loc,
               material->specular.r, material->specular.g, material->specular.b);
   glUniform1f(p->material_shininess_loc, material->shininess);
}

void
ter_shader_program_basic_load_material(TerShaderProgramBasic *p,
                                       TerMaterial *material)
{
   load_material(p, material);
}

void
ter_shader_program_basic_load_clip_plane(TerShaderProgramBasic *p,
                                         const glm::vec4 plane)
{
   glUniform4f(p->clip_plane_loc, plane.x, plane.y, plane.z, plane.w);
}

void
ter_shader_program_shadow_data_load(TerShaderProgramShadowData *p,
                                    const glm::mat4 *vp,
                                    float shadow_distance,
                                    float shadow_map_size,
                                    int shadow_pfc,
                                    unsigned shadow_map_sampler_unit)
{
   glUniformMatrix4fv(p->shadow_map_space_vp_loc, 1, GL_FALSE, &(*vp)[0][0]);
   glUniform1f(p->shadow_distance_loc, shadow_distance);
   glUniform1f(p->shadow_map_size_loc, shadow_map_size);
   glUniform1i(p->shadow_pfc_loc, shadow_pfc);
   glUniform1i(p->shadow_map_loc, shadow_map_sampler_unit);
}

static void
init_shadow_data(TerShaderProgramShadowData *p, unsigned programID)
{
   p->shadow_map_space_vp_loc =
      glGetUniformLocation(programID, "ShadowMapSpaceViewProjection");
   p->shadow_distance_loc = glGetUniformLocation(programID, "ShadowDistance");
   p->shadow_map_size_loc = glGetUniformLocation(programID, "ShadowMapSize");
   p->shadow_pfc_loc = glGetUniformLocation(programID, "ShadowPFC");   
   p->shadow_map_loc = glGetUniformLocation(programID, "ShadowMap");
}

TerShaderProgramTerrain *
ter_shader_program_terrain_new()
{
   unsigned programID = build_shader_program("../shaders/terrain.vert",
                                             "../shaders/terrain.frag");
   TerShaderProgramTerrain *p = g_new0(TerShaderProgramTerrain, 1);
   init_basic(&p->basic, programID);
   p->sampler_loc = glGetUniformLocation(programID, "SamplerTerrain");
   p->sampler_divisor_loc = glGetUniformLocation(programID, "SamplerCoordDivisor");
   return p;
}

TerShaderProgramTerrain *
ter_shader_program_terrain_shadow_new()
{
   unsigned programID = build_shader_program("../shaders/terrain-shadow.vert",
                                             "../shaders/terrain-shadow.frag");
   TerShaderProgramTerrain *p = g_new0(TerShaderProgramTerrain, 1);
   init_basic(&p->basic, programID);
   p->sampler_loc = glGetUniformLocation(programID, "SamplerTerrain");
   p->sampler_divisor_loc = glGetUniformLocation(programID, "SamplerCoordDivisor");
   init_shadow_data(&p->shadow, programID);
   return p;
}

void
ter_shader_program_terrain_load_sampler(TerShaderProgramTerrain *p,
                                        int unit, float divisor)
{
   glUniform1i(p->sampler_loc, unit);
   glUniform1f(p->sampler_divisor_loc, divisor);
}

TerShaderProgramSkybox *
ter_shader_program_skybox_new()
{
   unsigned programID = build_shader_program("../shaders/skybox.vert",
                                             "../shaders/skybox.frag");
   TerShaderProgramSkybox *p = g_new0(TerShaderProgramSkybox, 1);
   init_basic(&p->basic, programID);
   p->sampler_loc = glGetUniformLocation(programID, "Sampler");
   return p;
}

void
ter_shader_program_skybox_load_sampler(TerShaderProgramSkybox *p, int unit)
{
   glUniform1i(p->sampler_loc, unit);
}

static void
init_model_data(TerShaderProgramModelData *p, unsigned program)
{
   p->near_plane_loc =
      glGetUniformLocation(program, "NearPlane");
   p->far_clip_plane_loc =
      glGetUniformLocation(program, "FarClipPlane");
   p->far_render_plane_loc =
      glGetUniformLocation(program, "FarRenderPlane");

   for (int i = 0; i < TER_MODEL_MAX_MATERIALS * TER_MODEL_MAX_VARIANTS; i++) {
      char *name;
      name = g_strdup_printf("MaterialAmbient[%d]", i);
      p->ambient_loc[i] = glGetUniformLocation(program, name);
      name = g_strdup_printf("MaterialDiffuse[%d]", i);
      p->diffuse_loc[i] = glGetUniformLocation(program, name);
      name = g_strdup_printf("MaterialSpecular[%d]", i);
      p->specular_loc[i] = glGetUniformLocation(program, name);
      name = g_strdup_printf("MaterialShininess[%d]", i);
      p->shininess_loc[i] = glGetUniformLocation(program, name);
   }
}

TerShaderProgramModelSolid *
ter_shader_program_model_solid_new()
{
   unsigned programID = build_shader_program("../shaders/model-solid.vert",
                                             "../shaders/model-solid.frag");
   TerShaderProgramModelSolid *p = g_new0(TerShaderProgramModelSolid, 1);
   init_basic(&p->basic, programID);
   init_model_data(&p->model, programID);
   return p;
}

TerShaderProgramModelSolid *
ter_shader_program_model_solid_shadow_new()
{
   unsigned programID = build_shader_program("../shaders/model-solid-shadow.vert",
                                             "../shaders/model-solid-shadow.frag");
   TerShaderProgramModelSolid *p = g_new0(TerShaderProgramModelSolid, 1);
   init_basic(&p->basic, programID);
   init_shadow_data(&p->shadow, programID);
   init_model_data(&p->model, programID);
   return p;
}

TerShaderProgramModelTex *
ter_shader_program_model_tex_new()
{
   unsigned programID = build_shader_program("../shaders/model-tex.vert",
                                             "../shaders/model-tex.frag");
   TerShaderProgramModelTex *p = g_new0(TerShaderProgramModelTex, 1);
   init_basic(&p->basic, programID);
   init_model_data(&p->model, programID);
   p->tex_diffuse_loc[0] = glGetUniformLocation(programID, "TexDiffuse[0]");
   p->tex_diffuse_loc[1] = glGetUniformLocation(programID, "TexDiffuse[1]");
   p->tex_diffuse_loc[2] = glGetUniformLocation(programID, "TexDiffuse[2]");
   p->tex_diffuse_loc[3] = glGetUniformLocation(programID, "TexDiffuse[3]");
   return p;
}

TerShaderProgramModelTex *
ter_shader_program_model_tex_shadow_new()
{
   unsigned programID = build_shader_program("../shaders/model-tex-shadow.vert",
                                             "../shaders/model-tex-shadow.frag");
   TerShaderProgramModelTex *p = g_new0(TerShaderProgramModelTex, 1);
   init_basic(&p->basic, programID);
   init_shadow_data(&p->shadow, programID);
   init_model_data(&p->model, programID);
   p->tex_diffuse_loc[0] = glGetUniformLocation(programID, "TexDiffuse[0]");
   p->tex_diffuse_loc[1] = glGetUniformLocation(programID, "TexDiffuse[1]");
   p->tex_diffuse_loc[2] = glGetUniformLocation(programID, "TexDiffuse[2]");
   p->tex_diffuse_loc[3] = glGetUniformLocation(programID, "TexDiffuse[3]");
   return p;
}

void
ter_shader_program_model_tex_load_textures(TerShaderProgramModelTex *p,
                                           unsigned num_units)
{
   assert(num_units < 4);
   for (int i = 0; i < num_units; i++) {
      glUniform1i(p->tex_diffuse_loc[i], i);
   }
}

void
ter_shader_program_model_load_near_far_planes(TerShaderProgramModelData *p,
                                              float near, float far_clip,
                                              float far_render)
{
   glUniform1f(p->near_plane_loc, near);
   glUniform1f(p->far_clip_plane_loc, far_clip);
   glUniform1f(p->far_render_plane_loc, far_render);
}

void
ter_shader_program_model_load_materials(TerShaderProgramModelData *p,
                                        TerMaterial *materials,
                                        unsigned count)
{
   for (int i = 0; i < count; i++) {
      const TerMaterial &mat = materials[i];
      glUniform3f(p->ambient_loc[i],
                  mat.ambient.r, mat.ambient.g, mat.ambient.b);
      glUniform3f(p->diffuse_loc[i],
                  mat.diffuse.r, mat.diffuse.g, mat.diffuse.b);
      glUniform3f(p->specular_loc[i],
                  mat.specular.r, mat.specular.g, mat.specular.b);
      glUniform1f(p->shininess_loc[i], mat.shininess);
   }
}

TerShaderProgramWater *
ter_shader_program_water_new()
{
   unsigned programID = build_shader_program("../shaders/water.vert",
                                             "../shaders/water.frag");
   TerShaderProgramWater *p = g_new0(TerShaderProgramWater, 1);
   init_basic(&p->basic, programID);
   p->camera_position_loc = glGetUniformLocation(programID, "CameraPosition");
   p->tex_reflection_loc = glGetUniformLocation(programID, "ReflectionTex");
   p->tex_refraction_loc = glGetUniformLocation(programID, "RefractionTex");   
   p->tex_dudv_map_loc = glGetUniformLocation(programID, "DuDvMapTex");
   p->tex_normal_map_loc = glGetUniformLocation(programID, "NormalMapTex");
   p->tex_depth_loc = glGetUniformLocation(programID, "DepthTex");
   p->distortion_strength_loc =
      glGetUniformLocation(programID, "DistortionStrength");
   p->wave_factor_loc = glGetUniformLocation(programID, "WaveFactor");
   p->reflection_factor_loc =
      glGetUniformLocation(programID, "ReflectionFactor");
   p->near_plane_loc = glGetUniformLocation(programID, "NearPlane");
   p->far_plane_loc = glGetUniformLocation(programID, "FarPlane");
   init_shadow_data(&p->shadow, programID);
   return p;
}

void
ter_shader_program_water_load_textures(TerShaderProgramWater *p,
                                       unsigned reflection_unit,
                                       unsigned refraction_unit,
                                       unsigned dudv_unit,
                                       unsigned normal_unit,
                                       unsigned depth_unit)
{
   glUniform1i(p->tex_reflection_loc, reflection_unit);
   glUniform1i(p->tex_refraction_loc, refraction_unit);
   glUniform1i(p->tex_dudv_map_loc, dudv_unit);
   glUniform1i(p->tex_normal_map_loc, normal_unit);
   glUniform1i(p->tex_depth_loc, depth_unit);
}

void
ter_shader_program_water_load_distortion_strength(TerShaderProgramWater *p,
                                                  float distortion)
{
   glUniform1f(p->distortion_strength_loc, distortion);
}

void
ter_shader_program_water_load_wave_factor(TerShaderProgramWater *p,
                                          float factor)
{
   glUniform1f(p->wave_factor_loc, factor);
}

void
ter_shader_program_water_load_reflection_factor(TerShaderProgramWater *p,
                                                float factor)
{
   glUniform1f(p->reflection_factor_loc, factor);
}

void
ter_shader_program_water_load_camera_position(TerShaderProgramWater *p,
                                              const glm::vec3 pos)
{
   glUniform3f(p->camera_position_loc, pos.x, pos.y, pos.z);
}

void
ter_shader_program_water_load_near_far_planes(TerShaderProgramWater *p,
                                              float near, float far)
{
   glUniform1f(p->near_plane_loc, near);
   glUniform1f(p->far_plane_loc, far);
}

TerShaderProgramShadowMap *
ter_shader_program_shadow_map_new()
{
   unsigned programID = build_shader_program("../shaders/shadow-map.vert",
                                             "../shaders/shadow-map.frag");
   TerShaderProgramShadowMap *p = g_new0(TerShaderProgramShadowMap, 1);
   init_program(&p->prog, programID);
   p->projection_loc = glGetUniformLocation(programID, "Projection");
   p->view_loc = glGetUniformLocation(programID, "View");
   p->model_loc = glGetUniformLocation(programID, "Model");
  return p;
}

TerShaderProgramShadowMap *
ter_shader_program_shadow_map_instanced_new()
{
   unsigned programID = build_shader_program("../shaders/shadow-map-instanced.vert",
                                             "../shaders/shadow-map.frag");
   TerShaderProgramShadowMap *p = g_new0(TerShaderProgramShadowMap, 1);
   init_program(&p->prog, programID);
   p->projection_loc = glGetUniformLocation(programID, "Projection");
   p->view_loc = glGetUniformLocation(programID, "View");
  return p;
}

void
ter_shader_program_shadow_map_load_VP(TerShaderProgramShadowMap *p,
                                       const glm::mat4 *projection,
                                       const glm::mat4 *view)
{
   glUniformMatrix4fv(p->projection_loc, 1, GL_FALSE, &(*projection)[0][0]);
   glUniformMatrix4fv(p->view_loc, 1, GL_FALSE, &(*view)[0][0]);
}

void
ter_shader_program_shadow_map_load_MVP(TerShaderProgramShadowMap *p,
                                       const glm::mat4 *projection,
                                       const glm::mat4 *view,
                                       const glm::mat4 *model)
{
   glUniformMatrix4fv(p->projection_loc, 1, GL_FALSE, &(*projection)[0][0]);
   glUniformMatrix4fv(p->view_loc, 1, GL_FALSE, &(*view)[0][0]);
   glUniformMatrix4fv(p->model_loc, 1, GL_FALSE, &(*model)[0][0]);
}

TerShaderProgramBox *
ter_shader_program_box_new()
{
   unsigned programID = build_shader_program("../shaders/box.vert",
                                             "../shaders/box.frag");

   TerShaderProgramBox *p = g_new0(TerShaderProgramBox, 1);
   init_program(&p->prog, programID);

   p->mvp_loc = glGetUniformLocation(programID, "MVP");

   return p;
}

void
ter_shader_program_box_load_MVP(TerShaderProgramBox *p, glm::mat4 *MVP)
{
   glUniformMatrix4fv(p->mvp_loc, 1, GL_FALSE, &(*MVP)[0][0]);
}
