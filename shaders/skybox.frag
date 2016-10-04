#version 330 core

in vec3 vs_tex_coords;

uniform samplerCube Sampler;

out vec4 fs_color;

void main()
{
   fs_color = texture(Sampler, vs_tex_coords);
}
