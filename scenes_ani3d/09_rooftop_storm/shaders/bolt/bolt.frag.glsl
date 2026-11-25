#version 330 core

in vec2 data;
layout(location = 0) out vec4 FragColor;

void main()
{
	float x = data.y;
	float intensity = data.x;
	// White hot core + blue-violet halo
	float core = exp(-x * x * 35.0);
	float halo = exp(-x * x * 5.0);
	vec3 col = vec3(1.0) * core * 1.6 + vec3(0.45, 0.55, 1.0) * halo * 0.25;
	FragColor = vec4(col * intensity, 1.0);
}
