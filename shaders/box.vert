#version 330 core

/* Attributes */
layout(location = 0) in vec3 vertexPosition;

/* Uniforms */
uniform mat4 MVP;

void main() {
   gl_Position = MVP * vec4(vertexPosition, 1.0);
}
