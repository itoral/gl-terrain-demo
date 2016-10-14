#version 330 core

/* Inputs */
in vec2 tex_coords[11];

/* Uniforms */
uniform sampler2D Tex;

/* Outputs */
out vec4 fs_color;

void main()
{
   fs_color = vec4(0.0);
   fs_color += texture(Tex, tex_coords[0]) * 0.0093;
   fs_color += texture(Tex, tex_coords[1]) * 0.028002;
   fs_color += texture(Tex, tex_coords[2]) * 0.065984;
   fs_color += texture(Tex, tex_coords[3]) * 0.121703;
   fs_color += texture(Tex, tex_coords[4]) * 0.175713;
   fs_color += texture(Tex, tex_coords[5]) * 0.198596;
   fs_color += texture(Tex, tex_coords[6]) * 0.175713;
   fs_color += texture(Tex, tex_coords[7]) * 0.121703;
   fs_color += texture(Tex, tex_coords[8]) * 0.065984;
   fs_color += texture(Tex, tex_coords[9]) * 0.028002;
   fs_color += texture(Tex, tex_coords[10]) * 0.0093;
}
