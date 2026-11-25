#version 330 core

layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec2 vertex_data; // (intensity, across coordinate in [-1,1])

out vec2 data;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	data = vertex_data;
	gl_Position = projection * view * vec4(vertex_position, 1.0);
}
