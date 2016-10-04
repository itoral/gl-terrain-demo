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

/* Outputs */
out vec4 vs_pos;
out vec3 vs_normal;

void main() {
   mat4 MVP = Projection * View * Model;
   gl_Position = MVP * vec4(vertexPosition, 1.0);
   vs_pos = Model * vec4(vertexPosition, 1.0);
   vs_normal = normalize(ModelInvTransp * vertexNormal);
   gl_ClipDistance[0] = dot(vs_pos, ClipPlane);
}
