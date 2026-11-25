#version 330 core

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in float vertex_alpha;

out float alpha;
out float dist;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 camera_position;

void main()
{
	alpha = vertex_alpha;
	dist = length(vertex_position - camera_position);
	gl_Position = projection * view * vec4(vertex_position, 1.0);
}
