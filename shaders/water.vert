#version 330 core

const int CSM_LEVELS = 4;

/* Attributes */
layout(location = 0) in vec3 vertexPosition;

/* Uniforms */
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform mat4 PrevMVP;
uniform mat3 ModelInvTransp;

uniform vec3 CameraPosition;

uniform mat4 ShadowMapSpaceViewProjection[CSM_LEVELS];
uniform float ShadowDistance;
const float ShadowTransitionDistance = 10.0;

const float fog_density = 0.0125;
const float fog_gradient = 2.0;

/* Outputs */
out vec4 vs_pos;
out vec2 vs_uv;
out vec3 vs_camera_vector;
out float vs_visibility;
out vec4 vs_shadow_map_uv[CSM_LEVELS];
out float vs_dist_from_camera;
out vec4 vs_clip_pos;
out vec4 vs_prev_clip_pos;

void main() {
   vs_pos = Model * vec4(vertexPosition, 1.0);
   vec4 pos_from_camera = View * vs_pos;
   gl_Position = Projection * pos_from_camera;

   /* Replicate the dudv map across the water surface */
   vs_uv = vertexPosition.xz / 4.0f;
   vs_camera_vector = CameraPosition - vs_pos.xyz;

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

   vs_clip_pos = gl_Position;
   vs_prev_clip_pos = PrevMVP * vec4(vertexPosition, 1.0);
}
