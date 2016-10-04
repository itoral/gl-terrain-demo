#version 330 core

/* Attributes */
layout(location = 0) in vec3 vertexPosition;

/* Uniforms */
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform mat3 ModelInvTransp;

uniform vec3 CameraPosition;

uniform mat4 ShadowMapSpaceViewProjection;
uniform float ShadowDistance;
const float ShadowTransitionDistance = 10.0;

const float fog_density = 0.0125;
const float fog_gradient = 2.0;

/* Outputs */
out vec4 vs_pos;
out vec2 vs_uv;
out vec4 vs_clip_pos;
out vec3 vs_camera_vector;
out float vs_visibility;
out vec4 vs_shadow_map_uv;

void main() {
   vs_pos = Model * vec4(vertexPosition, 1.0);
   vec4 pos_from_camera = View * vs_pos;
   gl_Position = Projection * pos_from_camera;
   vs_clip_pos = gl_Position;

   /* Replicate the dudv map across the water surface */
   vs_uv = vertexPosition.xz / 4.0f;
   vs_camera_vector = CameraPosition - vs_pos.xyz;

   vs_shadow_map_uv = ShadowMapSpaceViewProjection * vs_pos;
   float shadow_distance =
      length(pos_from_camera.xyz) - (ShadowDistance - ShadowTransitionDistance);
   vs_shadow_map_uv.w =
      clamp(1.0 - shadow_distance / ShadowTransitionDistance, 0.0, 1.0);

   float distance_from_camera = length(pos_from_camera.xyz);
   vs_visibility = clamp(exp(-pow(distance_from_camera * fog_density, fog_gradient)),
                         0.0, 1.0);
}
