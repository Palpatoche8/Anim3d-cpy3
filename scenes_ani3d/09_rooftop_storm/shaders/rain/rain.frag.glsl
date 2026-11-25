#version 330 core

in float alpha;
in float dist;
layout(location = 0) out vec4 FragColor;

uniform vec3 flash_color;
uniform float flash_intensity;

void main()
{
	// Faint streaks, more visible close to the camera, lit by the flash
	float fade = clamp(1.0 - dist / 24.0, 0.0, 1.0);
	vec3 col = vec3(0.45, 0.50, 0.62) + flash_color * flash_intensity * 0.8;
	float a = alpha * fade * (0.22 + 0.35 * min(flash_intensity, 1.0));
	FragColor = vec4(col, a);
}
