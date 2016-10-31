#ifndef __DRV_SHADER_PROGRAM_H__
#define __DRV_SHADER_PROGRAM_H__

#define GLM_FORCE_RADIANS 1
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ter-light.h"
#include "ter-util.h"
#include "ter-shadow-renderer.h"

typedef struct {
   unsigned program;
} TerShaderProgram;

void ter_shader_program_free(TerShaderProgram *sh);

typedef struct {
   TerShaderProgram prog;

   unsigned projection_loc;
   unsigned model_loc;

   unsigned texture_loc;
} TerShaderProgramTile;

TerShaderProgramTile *ter_shader_program_tile_new();
void ter_shader_program_tile_load_MVP(TerShaderProgramTile *p,
                                      glm::mat4 *projection, glm::mat4 *model);
void ter_shader_program_tile_load_texture(TerShaderProgramTile *p,
                                          unsigned unit);

typedef struct {
   TerShaderProgram prog;

   unsigned projection_loc;
   unsigned view_loc;
   unsigned view_inv_loc;
   unsigned model_loc;
   unsigned model_it_loc;

   unsigned light_pos_loc;
   unsigned light_attenuation_loc;
   unsigned light_diffuse_loc;
   unsigned light_ambient_loc;
   unsigned light_specular_loc;

   unsigned material_diffuse_loc;
   unsigned material_ambient_loc;
   unsigned material_specular_loc;
   unsigned material_shininess_loc;

   unsigned clip_plane_loc;
   unsigned sky_color_loc;
} TerShaderProgramBasic;

TerShaderProgramBasic *ter_shader_program_basic_new();

void ter_shader_program_basic_load_MVP(TerShaderProgramBasic *p,
                                       const glm::mat4 *projection,
                                       const glm::mat4 *view,
                                       const glm::mat4 *view_inv,
                                       const glm::mat4 *model);

void ter_shader_program_basic_load_VP(TerShaderProgramBasic *p,
                                      const glm::mat4 *projection,
                                      const glm::mat4 *view,
                                      const glm::mat4 *view_inv);

void ter_shader_program_basic_load_M(TerShaderProgramBasic *p,
                                      const glm::mat4 *model);

void ter_shader_program_basic_load_light(TerShaderProgramBasic *p,
                                         TerLight *light);

void ter_shader_program_basic_load_sky_color(TerShaderProgramBasic *p,
                                             glm::vec3 *color);

void ter_shader_program_basic_load_material(TerShaderProgramBasic *p,
                                            TerMaterial *material);

void ter_shader_program_basic_load_clip_plane(TerShaderProgramBasic *p,
                                              const glm::vec4 plane);

typedef struct {
   unsigned shadow_map_space_vp_loc[TER_MAX_CSM_LEVELS];
   unsigned shadow_map_loc[TER_MAX_CSM_LEVELS];
   unsigned shadow_csm_end_loc[TER_MAX_CSM_LEVELS];
   unsigned shadow_map_size_loc[TER_MAX_CSM_LEVELS];
   unsigned shadow_num_csm_levels_loc;
   unsigned shadow_distance_loc;
   unsigned shadow_pfc_loc;
} TerShaderProgramShadowData;

void ter_shader_program_shadow_data_load(TerShaderProgramShadowData *p,
                                         const glm::mat4 *vp,
                                         float shadow_distance,
                                         float shadow_map_size,
                                         int shadow_pfc,
                                         unsigned shadow_map_sampler_unit);

void ter_shader_program_shadow_data_load_(TerShaderProgramShadowData *p,
                                          TerShadowRenderer *sr,
                                          unsigned unit);

typedef struct {
   TerShaderProgramBasic basic;
   unsigned sampler_loc;
   unsigned sampler_divisor_loc;
   TerShaderProgramShadowData shadow;
   unsigned prev_mvp_loc;
} TerShaderProgramTerrain;

TerShaderProgramTerrain *ter_shader_program_terrain_new();
TerShaderProgramTerrain *ter_shader_program_terrain_shadow_new();

void ter_shader_program_terrain_load_sampler(TerShaderProgramTerrain *p,
                                             int unit, float divisor);

void ter_shader_program_terrain_load_prev_MVP(TerShaderProgramTerrain *p,
                                              glm::mat4 *mat);

typedef struct {
   TerShaderProgramBasic basic;
   unsigned sampler_loc;
} TerShaderProgramSkybox;

TerShaderProgramSkybox *ter_shader_program_skybox_new();

void ter_shader_program_skybox_load_sampler(TerShaderProgramSkybox *p, int unit);

typedef struct {
   unsigned ambient_loc[TER_MODEL_MAX_MATERIALS * TER_MODEL_MAX_VARIANTS];
   unsigned diffuse_loc[TER_MODEL_MAX_MATERIALS * TER_MODEL_MAX_VARIANTS];
   unsigned specular_loc[TER_MODEL_MAX_MATERIALS * TER_MODEL_MAX_VARIANTS];   
   unsigned shininess_loc[TER_MODEL_MAX_MATERIALS * TER_MODEL_MAX_VARIANTS];
   unsigned near_plane_loc, far_clip_plane_loc, far_render_plane_loc;
} TerShaderProgramModelData;

typedef struct {
   TerShaderProgramBasic basic;
   TerShaderProgramModelData model;   
} TerShaderProgramModel;

typedef struct {
   TerShaderProgramBasic basic;
   TerShaderProgramModelData model;
   TerShaderProgramShadowData shadow;
} TerShaderProgramModelSolid;

TerShaderProgramModelSolid *ter_shader_program_model_solid_new();
TerShaderProgramModelSolid *ter_shader_program_model_solid_shadow_new();

typedef struct {
   TerShaderProgramBasic basic;
   TerShaderProgramModelData model;
   unsigned tex_diffuse_loc[TER_MODEL_MAX_TEXTURES];
   TerShaderProgramShadowData shadow;
} TerShaderProgramModelTex;

TerShaderProgramModelTex *ter_shader_program_model_tex_new();
TerShaderProgramModelTex *ter_shader_program_model_tex_shadow_new();

void ter_shader_program_model_tex_load_textures(TerShaderProgramModelTex *p,
                                                unsigned num_units);

void ter_shader_program_model_load_near_far_planes(TerShaderProgramModelData *p,
                                                   float near, float far_clip,
                                                   float far_render);

void ter_shader_program_model_load_materials(TerShaderProgramModelData *p,
                                             TerMaterial *materials,
                                             unsigned count);

typedef struct {
   TerShaderProgramBasic basic;
   unsigned camera_position_loc;
   unsigned distortion_strength_loc;
   unsigned wave_factor_loc;
   unsigned reflection_factor_loc;
   unsigned tex_reflection_loc;
   unsigned tex_refraction_loc;
   unsigned tex_dudv_map_loc;
   unsigned tex_normal_map_loc;
   unsigned tex_depth_loc;
   unsigned near_plane_loc, far_plane_loc;
   TerShaderProgramShadowData shadow;
} TerShaderProgramWater;

TerShaderProgramWater *ter_shader_program_water_new();
void ter_shader_program_water_load_textures(TerShaderProgramWater *p,
                                            unsigned reflection_unit,
                                            unsigned refraction_unit,
                                            unsigned dudv_unit,
                                            unsigned normal_unit,
                                            unsigned depth_loc);

void ter_shader_program_water_load_distortion_strength(TerShaderProgramWater *p,
                                                       float distortion);
void ter_shader_program_water_load_wave_factor(TerShaderProgramWater *p,
                                               float factor);

void ter_shader_program_water_load_reflection_factor(TerShaderProgramWater *p,
                                                     float factor);

void ter_shader_program_water_load_camera_position(TerShaderProgramWater *p,
                                                   const glm::vec3 position);

void ter_shader_program_water_load_near_far_planes(TerShaderProgramWater *p,
                                                   float near, float far);

typedef struct {
   TerShaderProgram prog;
   unsigned projection_loc;
   unsigned view_loc;
   unsigned model_loc;
} TerShaderProgramShadowMap;

TerShaderProgramShadowMap *ter_shader_program_shadow_map_new();
TerShaderProgramShadowMap *ter_shader_program_shadow_map_instanced_new();

void ter_shader_program_shadow_map_load_VP(TerShaderProgramShadowMap *p,
                                            const glm::mat4 *projection,
                                            const glm::mat4 *view);

void ter_shader_program_shadow_map_load_MVP(TerShaderProgramShadowMap *p,
                                            const glm::mat4 *projection,
                                            const glm::mat4 *view,
                                            const glm::mat4 *model);

typedef struct {
   TerShaderProgram prog;
   unsigned mvp_loc;
} TerShaderProgramBox;

TerShaderProgramBox *ter_shader_program_box_new();
void ter_shader_program_box_load_MVP(TerShaderProgramBox *p,
                                     glm::mat4 *MVP);

typedef struct {
   TerShaderProgram prog;
   unsigned texture_loc;
} TerShaderProgramFilterSimple;

TerShaderProgramFilterSimple *ter_shader_program_filter_simple_new(
   const char *vs, const char *fs);

void ter_shader_program_filter_simple_load(
   TerShaderProgramFilterSimple *p, unsigned unit);

typedef struct {
   TerShaderProgramFilterSimple simple;
   unsigned lum_factor_loc;
} TerShaderProgramFilterBrightnessSelect;

TerShaderProgramFilterBrightnessSelect *
ter_shader_program_filter_brightness_select_new(const char *vs, const char *fs);

void ter_shader_program_filter_brightness_select_load(
   TerShaderProgramFilterBrightnessSelect *p, unsigned unit, float lum_factor);

typedef struct {
   TerShaderProgramFilterSimple simple;
   unsigned dim_loc;
} TerShaderProgramFilterBlur;

TerShaderProgramFilterBlur *ter_shader_program_filter_blur_new(
   const char *vs, const char *fs);

void ter_shader_program_filter_blur_load(
   TerShaderProgramFilterBlur *p, unsigned unit, unsigned dim);

typedef struct {
   TerShaderProgramFilterSimple simple;
   unsigned texture2_loc;
} TerShaderProgramFilterCombine;

TerShaderProgramFilterCombine *ter_shader_program_filter_combine_new(
   const char *vs, const char *fs);

void ter_shader_program_filter_combine_load(
   TerShaderProgramFilterCombine *p, unsigned unit0, unsigned unit1);

typedef struct {
   TerShaderProgramFilterSimple simple;
   unsigned motion_texture_loc;
   unsigned motion_divisor_loc;
} TerShaderProgramFilterMotionBlur;

TerShaderProgramFilterMotionBlur *ter_shader_program_filter_motion_blur_new(
   const char *vs, const char *fs);

void ter_shader_program_filter_motion_blur_load(
   TerShaderProgramFilterMotionBlur *p, unsigned unit0, unsigned unit1,
   float divisor);

#endif
