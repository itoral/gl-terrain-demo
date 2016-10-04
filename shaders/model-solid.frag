#version 330 core

/* Inputs */
in vec4 vs_pos;
in vec3 vs_normal;
flat in int vs_mat_idx;

/* Uniforms */
uniform mat4 ViewInv;

uniform vec4 LightPosition;
uniform float LightAttenuation;
uniform vec3 LightDiffuse;
uniform vec3 LightAmbient;
uniform vec3 LightSpecular;

uniform vec3 MaterialDiffuse[16];
uniform vec3 MaterialAmbient[16];
uniform vec3 MaterialSpecular[16];
uniform float MaterialShininess[16];

uniform float NearPlane;
uniform float FarClipPlane;
uniform float FarRenderPlane;

/* Outputs */
out vec4 fs_color;

void main()
{
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

   vec3 vs_ambient = MaterialAmbient[vs_mat_idx];
   vec3 vs_diffuse = MaterialDiffuse[vs_mat_idx];
   vec3 vs_specular = MaterialSpecular[vs_mat_idx];
   float vs_shininess = MaterialShininess[vs_mat_idx];

   /* Diffuse */
   vec3 diffuse = attenuation * LightDiffuse * vs_diffuse *
      max(0.0, dot(normal, light_dir));

   /* Ambient */
   vec3 ambient = LightAmbient * (vs_ambient + vs_diffuse);

   /* Specular */
   vec3 specular;
   if (vs_shininess == 0.0 || dot(normal, light_dir) < 0.0) {
      specular = vec3(0.0, 0.0, 0.0);
   } else {
      vec3 view_dir =
         normalize(vec3(ViewInv * vec4(0.0, 0.0, 0.0, 1.0) - vs_pos));
      vec3 reflection_dir = reflect(-light_dir, normal);
      float shine_factor = dot(reflection_dir, view_dir);
      specular =
         attenuation * LightSpecular * vs_specular *
            pow(max(0.0, shine_factor), vs_shininess);
   }

   /* Alpha (objects close to the far clipping plane fade-in progressively) */
   float near = NearPlane;
   float far = FarRenderPlane;
   float fade_dist = 15.0;
   float depth =
      2.0 * near * far / (far + near - (2.0 * gl_FragCoord.z - 1.0) * (far - near));
   float alpha = 1.0 - clamp((depth - (FarClipPlane - fade_dist)) / fade_dist, 0.0, 1.0);

   fs_color = vec4(diffuse + ambient + specular, alpha);
}
