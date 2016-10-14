#version 330 core

/* Inputs */
in vec2 tex_coords;

/* Uniforms */
uniform sampler2D Tex;
uniform float LuminanceFactor;

/* Outputs */
out vec4 fs_color;

void main()
{
   vec4 texel = texture(Tex, tex_coords);
   float lum = dot(texel.rgb, vec3(0.2126, 0.7152, 0.0722));
   fs_color = texel * pow(lum, LuminanceFactor);
}
