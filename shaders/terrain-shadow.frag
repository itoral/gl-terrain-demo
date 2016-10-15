#version 330 core

const int CSM_LEVELS = 4;

/* Inputs */
in vec4 vs_pos;
in vec3 vs_normal;
in vec4 vs_shadow_map_uv[CSM_LEVELS];
in float vs_dist_from_camera;
in float vs_visibility;

/* Uniforms */
uniform mat4 ViewInv;

uniform vec4 LightPosition;
uniform float LightAttenuation;
uniform vec3 LightDiffuse;
uniform vec3 LightAmbient;
uniform vec3 LightSpecular;

uniform vec3 MaterialSpecular;
uniform float MaterialShininess;

uniform sampler2D SamplerTerrain;
uniform float SamplerCoordDivisor;

uniform sampler2DShadow ShadowMap[CSM_LEVELS];
uniform float ShadowCSMEndClipSpace[CSM_LEVELS];
uniform float ShadowMapSize[CSM_LEVELS];
uniform int ShadowCSMLevels;
uniform int ShadowPFC;
const float ShadowAcneBias = 0.002;

uniform vec3 SkyColor;

/* Outputs */
out vec4 fs_color;

float sample_shadow_map(int level, vec3 shadow_coords)
{
   /* Can't use non-uniform expressions with sampler arrays :-( */
   if (level == 0)
      return texture(ShadowMap[0], shadow_coords);
   else if (level == 1)
      return texture(ShadowMap[1], shadow_coords);
   else if (level == 2)
      return texture(ShadowMap[2], shadow_coords);

   return texture(ShadowMap[3], shadow_coords);
}

float compute_shadow_factor(float dp)
{
   for (int level = 0; level < CSM_LEVELS; level++) {
      if (vs_dist_from_camera <= ShadowCSMEndClipSpace[level]) {
         float kernel_size = ShadowPFC * 2.0 + 1.0;
         float num_samples =  kernel_size * kernel_size;
         float texel_size = 1.0 / ShadowMapSize[level];
         float shadowed_texels = 0.0;
         float bias = ShadowAcneBias * tan(acos(dp));
         float ref_dist = vs_shadow_map_uv[level].z - bias;

         for (int x = -ShadowPFC; x <= ShadowPFC; x++) {
            for (int y = -ShadowPFC; y <= ShadowPFC; y++) {
               vec3 shadow_coords =
                  vec3(vs_shadow_map_uv[level].xy + vec2(x, y) * texel_size, ref_dist);
               shadowed_texels += sample_shadow_map(level, shadow_coords);
            }
         }

         return 1.0 - shadowed_texels / num_samples * vs_shadow_map_uv[level].w;
      }
   }

   /* Fragment outside shadow map, no shadowing */
   return 1.0;
}

void main()
{
   /* Compute lighting parameters */
   vec3 light_dir;
   float attenuation;

   /* Light direction and attenuation factor */
   if (LightPosition.w == 0.0f) {
      /* Directional light */
      light_dir = normalize(vec3(LightPosition));
      attenuation = 1.0f;
   } else {
      /* Positional light */
      vec3 pos_to_light = vec3(LightPosition - vs_pos);
      float distance = length(pos_to_light);
      light_dir = normalize(pos_to_light);
      attenuation = 1.0 / (LightAttenuation * distance);
   }

   vec3 normal = normalize(vs_normal);
   float dp = dot(normal, light_dir);

   /* Is this pixel in the shade? Take mutiple samples to soften shadow edges */
   float shadow_factor = compute_shadow_factor(dp);

   /* Diffuse */
   vec2 tex_coords = vec2(vs_pos.x, vs_pos.z) / SamplerCoordDivisor;
   vec3 texel = vec3(texture(SamplerTerrain, tex_coords));
   vec3 diffuse = attenuation * LightDiffuse * texel * max(0.0, dp) * shadow_factor;

   /* Ambient */
   vec3 ambient = LightAmbient * texel;

   /* Specular */
   vec3 specular;
   if (MaterialShininess == 0.0 || dp < 0.0) {
      specular = vec3(0.0, 0.0, 0.0);
   } else {
      vec3 view_dir =
         normalize(vec3(ViewInv * vec4(0.0, 0.0, 0.0, 1.0) - vs_pos));
      vec3 reflection_dir = reflect(-light_dir, normal);
      float shine_factor = dot(reflection_dir, view_dir);
      specular =
         attenuation * LightSpecular * MaterialSpecular *
            pow(max(0.0, shine_factor), MaterialShininess) *
               (shadow_factor / 1.0);
   }

   vec3 light_color = diffuse + ambient + specular;
   vec3 final_color = mix(SkyColor, light_color, vs_visibility);
   fs_color = vec4(final_color, 1.0);
}
