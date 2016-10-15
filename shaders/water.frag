#version 330 core

const int CSM_LEVELS = 4;

/* Inputs */
in vec4 vs_pos;
in vec2 vs_uv;
in vec4 vs_clip_pos;
in vec3 vs_camera_vector;
in float vs_visibility;
in vec4 vs_shadow_map_uv[CSM_LEVELS];
in float vs_dist_from_camera;

/* Uniforms */
uniform mat4 ViewInv;

uniform vec4 LightPosition;
uniform float LightAttenuation;
uniform vec3 LightDiffuse;
uniform vec3 LightAmbient;
uniform vec3 LightSpecular;

uniform vec3 MaterialDiffuse;
uniform vec3 MaterialAmbient;
uniform vec3 MaterialSpecular;
uniform float MaterialShininess;

uniform float WaveFactor;
uniform float DistortionStrength;
uniform float ReflectionFactor;
uniform sampler2D ReflectionTex;
uniform sampler2D RefractionTex;
uniform sampler2D DuDvMapTex;
uniform sampler2D NormalMapTex;
uniform sampler2D DepthTex;

uniform float NearPlane;
uniform float FarPlane;

uniform sampler2DShadow ShadowMap[CSM_LEVELS];
uniform float ShadowCSMEndClipSpace[CSM_LEVELS];
uniform float ShadowMapSize[CSM_LEVELS];
uniform int ShadowCSMLevels;
uniform int ShadowPFC;
const float ShadowDistortionDivisor = 5.0;

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

/*
 * We don't use an acne bias for the shadows with the water because
 * the water itself does not cast a shadow, so there should be no
 * acne at all.
 */
float compute_shadow_factor(vec2 distortion)
{
   for (int level = 0; level < CSM_LEVELS; level++) {
      if (vs_dist_from_camera <= ShadowCSMEndClipSpace[level]) {
         float kernel_size = ShadowPFC * 2.0 + 1.0;
         float num_samples =  kernel_size * kernel_size;
         float texel_size = 1.0 / ShadowMapSize[level];
         float shadowed_texels = 0.0;
         float ref_dist = vs_shadow_map_uv[level].z;

         /* Apply a small distortion to the shadow on the water */
         vec2 distorted_coords = vs_shadow_map_uv[level].xy + distortion * 0.5;

         for (int x = -ShadowPFC; x <= ShadowPFC; x++) {
            for (int y = -ShadowPFC; y <= ShadowPFC; y++) {
               vec3 shadow_coords =
                  vec3(distorted_coords + vec2(x, y) * texel_size, ref_dist);
               shadowed_texels += sample_shadow_map(level, shadow_coords);
            }
         }

         return 1.0 - shadowed_texels / num_samples * vs_shadow_map_uv[level].w;
      }
   }

   /* Fragment outside shadow map, no shadowing */
   return 1.0;
}
/*
float compute_shadow_factor(vec2 distortion)
{
   float kernel_size = ShadowPFC * 2.0 + 1.0;
   float num_samples =  kernel_size * kernel_size;
   float texel_size = 1.0 / ShadowMapSize;

   vec2 distorted_coords = vs_shadow_map_uv.xy + distortion * 0.5;

   float shadowed_texels = 0.0;
   for (int x = -ShadowPFC; x <= ShadowPFC; x++) {
      for (int y = -ShadowPFC; y <= ShadowPFC; y++) {
         vec3 shadow_coords =
            vec3(distorted_coords + vec2(x, y) * texel_size, vs_shadow_map_uv.z);
         shadowed_texels += texture(ShadowMap, shadow_coords);

      }
   }

   return 1.0 - shadowed_texels / num_samples * vs_shadow_map_uv.w;
}
*/
void main()
{
   /* ========== Compute lightning parameters ============ */

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

   /* ============== Compute water Depth =================== */

   /* Map fragment position to screen coordinates [0, 1] */
   vec2 ndc = (vs_clip_pos.xy / vs_clip_pos.w) / 2.0 + 0.5;

   float near = NearPlane;
   float far = FarPlane;
   float depth = texture(DepthTex, vec2(ndc.x, ndc.y)).r;
   float floor_dist =
      2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));

   depth = gl_FragCoord.z;
   float water_dist =
      2.0 * near * far / (far + near - (2.0 * depth - 1.0) * (far - near));

   float water_depth = floor_dist - water_dist;

   /* ========== Compute water distortion =========== */

   vec2 distortion_uv =
      texture(DuDvMapTex, vec2(vs_uv.x + WaveFactor, vs_uv.y)).rg * 0.1;
   distortion_uv = vs_uv + vec2(distortion_uv.x, distortion_uv.y + WaveFactor);

   vec2 distortion = texture(DuDvMapTex, distortion_uv).rg * 2.0 - 1.0;
   distortion *= DistortionStrength;
   distortion *= clamp(water_depth, 0.0, 1.0); /* Do not distort edges */

   /* ============== Compute shadowing ============== */

   float shadow_factor =
      compute_shadow_factor(distortion / ShadowDistortionDivisor);

   /* ================ Compute normal =============== */

   vec4 normal_map = texture(NormalMapTex, distortion_uv);
   vec3 normal = normalize(vec3(normal_map.r * 2.0 - 1.0,
                                normal_map.b * 3.0,
                                normal_map.g * 2.0 - 1.0));

   /* ========== Compute reflection / refraction ========== */

   vec2 reflection_uv = vec2(ndc.x, -ndc.y);
   vec2 refraction_uv = vec2(ndc.x, ndc.y);

   reflection_uv.x = clamp(reflection_uv.x + distortion.x, 0.0, 1.0);
   reflection_uv.y = clamp(reflection_uv.y + distortion.y, -1.0, 0.0);
   refraction_uv = clamp(refraction_uv + distortion, 0.0, 1.0);

   vec4 reflection_color = texture(ReflectionTex, reflection_uv);
   vec4 refraction_color = texture(RefractionTex, refraction_uv);

   vec3 view_vector = normalize(vs_camera_vector);
   float reflection_factor = pow(dot(view_vector, normal), ReflectionFactor);

   vec3 water_color =
      mix(vec3(mix(reflection_color, refraction_color, reflection_factor)),
          MaterialDiffuse, 0.3);

   /* =============== Apply lightning ============== */

   /* Diffuse */
   float dp = dot(normal, light_dir);
   vec3 diffuse = attenuation * LightDiffuse * water_color *
      max(0.0, dp) * shadow_factor;

   /* Ambient */
   vec3 ambient = LightAmbient * water_color;

   /* Specular */
   vec3 specular;
   if (MaterialShininess == 0.0 || dp < 0.0) {
      specular = vec3(0.0, 0.0, 0.0);
   } else {
      vec3 reflection_dir = reflect(-light_dir, normal);
      float shine_factor = max(dot(reflection_dir, view_vector), 0.0);
      specular =
         attenuation * LightSpecular * MaterialSpecular *
            pow(shine_factor, MaterialShininess) * shadow_factor;
   }

   vec3 light_color = diffuse + ambient + specular;
   vec3 final_color = mix(SkyColor, light_color, vs_visibility);

   fs_color = vec4(final_color, 1.0);
   fs_color.a = clamp(water_depth, 0.0, 1.0);
}
