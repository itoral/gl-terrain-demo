#version 330 core

in vec3 vs_tex_coords;
in vec4 vs_clip_pos;
in vec4 vs_prev_clip_pos;

uniform samplerCube Sampler;

out vec4 fs_color;
out vec4 fs_motion_vector;

void main()
{
   fs_color = texture(Sampler, vs_tex_coords);

   /* Motion vector (0.5 means no motion) */
   vec3 ndc_pos = (vs_clip_pos / vs_clip_pos.w).xyz;
   vec3 prev_ndc_pos = (vs_prev_clip_pos / vs_prev_clip_pos.w).xyz;
   fs_motion_vector = vec4((ndc_pos - prev_ndc_pos).xy + 0.5, 0, 1);
}
