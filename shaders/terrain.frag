#version 330 core

/* Inputs */
in vec4 vs_pos;
in vec3 vs_normal;

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

/* Outputs */
out vec4 fs_color;

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

   /* Diffuse */
   vec2 tex_coords = vec2(vs_pos.x, vs_pos.z) / SamplerCoordDivisor;
   vec3 texel = vec3(texture(SamplerTerrain, tex_coords));
   vec3 diffuse = attenuation * LightDiffuse * texel *
      max(0.0, dot(normal, light_dir));

   /* Ambient */
   vec3 ambient = LightAmbient * texel;

   /* Specular */
   vec3 specular;
   if (MaterialShininess == 0.0 || dot(normal, light_dir) < 0.0) {
      specular = vec3(0.0, 0.0, 0.0);
   } else {
      vec3 view_dir =
         normalize(vec3(ViewInv * vec4(0.0, 0.0, 0.0, 1.0) - vs_pos));
      vec3 reflection_dir = reflect(-light_dir, normal);
      float shine_factor = dot(reflection_dir, view_dir);
      specular =
         attenuation * LightSpecular * MaterialSpecular *
            pow(max(0.0, shine_factor), MaterialShininess);
   }

   fs_color = vec4(diffuse + ambient + specular, 1.0);
}
