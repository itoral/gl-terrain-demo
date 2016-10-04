#version 330 core

/* Inputs */
in vec2 tex_coords;

/* Uniforms */
uniform sampler2D Tex;

/* Outputs */
out vec4 fs_color;

void main()
{
   fs_color = texture(Tex, tex_coords);
//   fs_color = vec4(1, 0, 0, 1);
}
