#version 330 core

/* Inputs */
in vec2 tex_coords;

/* Uniforms */
uniform sampler2D Tex;
uniform sampler2D Tex2;

/* Outputs */
out vec4 fs_color;

void main()
{
   vec4 texel1 = texture(Tex, tex_coords);
   vec4 texel2 = texture(Tex2, tex_coords);
   fs_color = texel1 + texel2 * 2.0;
}
