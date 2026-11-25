#version 330 core

// Standard vertex shader for the lit meshes (also used by the buildings)
layout (location = 0) in vec3 vertex_position;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec3 vertex_color;
layout (location = 3) in vec2 vertex_uv;

out struct fragment_data
{
	vec3 position; // world space
	vec3 normal;   // world space
	vec3 color;
	vec2 uv;
} fragment;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec4 position = model * vec4(vertex_position, 1.0);
	mat3 normal_matrix = transpose(inverse(mat3(model)));

	fragment.position = position.xyz;
	fragment.normal = normal_matrix * vertex_normal;
	fragment.color = vertex_color;
	fragment.uv = vertex_uv;

	gl_Position = projection * view * position;
}
