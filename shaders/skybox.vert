#version 330 core

layout(location = 0) in vec3 vertexPosition;

uniform mat4 Projection;
uniform mat4 View;
uniform mat4 Model;

out vec3 vs_tex_coords;

void main()
{
   gl_Position = Projection * View * Model * vec4(vertexPosition, 1.0);
   vs_tex_coords = vertexPosition;
}
