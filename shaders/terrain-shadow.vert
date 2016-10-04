#version 330 core

/* Attributes */
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec3 vertexNormal;

/* Uniforms */
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;
uniform mat3 ModelInvTransp;
uniform vec4 ClipPlane;

uniform mat4 ShadowMapSpaceViewProjection;
uniform float ShadowDistance;
const float ShadowTransitionDistance = 10.0;

const float fog_density = 0.0125;
const float fog_gradient = 2.0;

/* Outputs */
out vec4 vs_pos;
out vec3 vs_normal;
out vec4 vs_shadow_map_uv;
out float vs_visibility;

void main() {
   vs_pos = Model * vec4(vertexPosition, 1.0);
   vec4 pos_from_camera = View * vs_pos;
   gl_Position = Projection * pos_from_camera;
   vs_normal = normalize(ModelInvTransp * vertexNormal);
   gl_ClipDistance[0] = dot(vs_pos, ClipPlane);

   vs_shadow_map_uv = ShadowMapSpaceViewProjection * vs_pos;
   float shadow_distance =
      length(pos_from_camera.xyz) - (ShadowDistance - ShadowTransitionDistance);
   vs_shadow_map_uv.w =
      clamp(1.0 - shadow_distance / ShadowTransitionDistance, 0.0, 1.0);

   float distance_from_camera = length(pos_from_camera.xyz);
   vs_visibility = clamp(exp(-pow(distance_from_camera * fog_density, fog_gradient)),
                         0.0, 1.0);
}
