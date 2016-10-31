#version 330 core

layout(location = 0) in vec3 vertexPosition;

uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Model;
uniform mat4 PrevMVP;

out vec3 vs_tex_coords;
out vec4 vs_clip_pos;
out vec4 vs_prev_clip_pos;

void main()
{
   gl_Position = Projection * View * Model * vec4(vertexPosition, 1.0);
   vs_tex_coords = vertexPosition;

   vs_clip_pos = gl_Position;
   vs_prev_clip_pos = PrevMVP * vec4(vertexPosition, 1.0);
}
