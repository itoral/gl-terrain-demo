#version 330 core

/* Attributes */
layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec2 vertexTexCoords;

/* Uniforms */
uniform float Dim;

/* Output */
out vec2 tex_coords[11];

void main() {
   gl_Position = vec4(vertexPosition, 0.0, 1.0);
   vec2 center_tex_coords = vertexPosition * 0.5 + 0.5;
   float pixel_size = 1.0 / Dim;

   for (int i = -5; i <= 5; i++) {
      tex_coords[i + 5] = center_tex_coords + vec2(0.0, pixel_size * i);
   }
}
