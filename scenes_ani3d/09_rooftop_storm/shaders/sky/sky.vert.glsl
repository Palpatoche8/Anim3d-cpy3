#version 330 core

// Sky dome: a sphere centered on the camera, drawn first without depth writes
layout (location = 0) in vec3 vertex_position;

out vec3 direction;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	direction = vertex_position;
	// Remove the translation of the view matrix: the sky follows the camera
	mat4 view_rotation = mat4(mat3(view));
	vec4 p = projection * view_rotation * vec4(vertex_position, 1.0);
	gl_Position = p.xyww; // depth = 1 (far plane)
}
