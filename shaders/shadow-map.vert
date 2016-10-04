#version 330 core

/* Attributes */
layout(location = 0) in vec3 vertexPosition;

/* Uniforms */
uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

void main() {
   mat4 MVP = Projection * View * Model;
   gl_Position = MVP * vec4(vertexPosition, 1.0);
}
