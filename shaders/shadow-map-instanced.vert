#version 330 core

/* Attributes */
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in mat4 Model;

/* Uniforms */
uniform mat4 View;
uniform mat4 Projection;

void main() {
   mat4 MVP = Projection * View * Model;
   gl_Position = MVP * vec4(vertexPosition, 1.0);
}
