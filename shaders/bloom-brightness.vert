#version 330 core

/* Attributes */
layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;

/* Output */
out vec2 tex_coords;

void main() {
   gl_Position = vec4(vertexPosition, 0.0, 1.0);
   tex_coords = vertexTexCoords;
}
