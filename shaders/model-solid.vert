#version 330 core

/* Attributes */
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in mat4 Model;
layout(location = 5) in int VariantIdx;
layout(location = 6) in vec3 vertexNormal;
layout(location = 7) in int vertexMatIdx;


/* Uniforms */
uniform mat4 View;
uniform mat4 Projection;
uniform vec4 ClipPlane;

/* Outputs */
out vec4 vs_pos;
out vec3 vs_normal;
flat out int vs_mat_idx;

void main() {
   mat3 ModelInvTransp = transpose(inverse(mat3(Model)));

   mat4 MVP = Projection * View * Model;
   gl_Position = MVP * vec4(vertexPosition, 1.0);
   vs_pos = Model * vec4(vertexPosition, 1.0);
   vs_normal = normalize(ModelInvTransp * vertexNormal);
   vs_mat_idx = vertexMatIdx + VariantIdx;
   gl_ClipDistance[0] = dot(vs_pos, ClipPlane);
}
