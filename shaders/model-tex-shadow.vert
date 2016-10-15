#version 330 core

const int CSM_LEVELS = 4;

/* Attributes */
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in mat4 Model;
layout(location = 5) in int VariantIdx;
layout(location = 6) in vec3 vertexNormal;
layout(location = 7) in int vertexMatIdx;
layout(location = 8) in vec2 vertexUV;
layout(location = 9) in int vertexSampler;

/* Uniforms */
uniform mat4 View;
uniform mat4 Projection;
uniform vec4 ClipPlane;

uniform mat4 ShadowMapSpaceViewProjection[CSM_LEVELS];
uniform float ShadowDistance;
const float ShadowTransitionDistance = 10.0;

const float fog_density = 0.0125;
const float fog_gradient = 2.0;

/* Outputs */
out vec4 vs_pos;
out vec3 vs_normal;
out vec2 vs_uv;
flat out int vs_mat_idx;
flat out int vs_sampler_index;
out vec4 vs_shadow_map_uv[CSM_LEVELS];
out float vs_dist_from_camera;
out float vs_visibility;

void main() {
   mat3 ModelInvTransp = transpose(inverse(mat3(Model)));

   vs_pos = Model * vec4(vertexPosition, 1.0);
   vec4 pos_from_camera = View * vs_pos;
   gl_Position = Projection * pos_from_camera;
   vs_normal = normalize(ModelInvTransp * vertexNormal);
   vs_uv = vertexUV;
   vs_mat_idx = vertexMatIdx + VariantIdx;
   vs_sampler_index = vertexSampler;
   gl_ClipDistance[0] = dot(vs_pos, ClipPlane);

   float distance_from_camera = length(pos_from_camera.xyz);
   float shadow_distance =
      distance_from_camera - (ShadowDistance - ShadowTransitionDistance);
   for (int i = 0; i < CSM_LEVELS; i++) {
      vs_shadow_map_uv[i] = ShadowMapSpaceViewProjection[i] * vs_pos;
      vs_shadow_map_uv[i].w =
         clamp(1.0 - shadow_distance / ShadowTransitionDistance, 0.0, 1.0);
   }
   vs_dist_from_camera = distance_from_camera;

   vs_visibility = clamp(exp(-pow(distance_from_camera * fog_density, fog_gradient)),
                         0.0, 1.0);
}
