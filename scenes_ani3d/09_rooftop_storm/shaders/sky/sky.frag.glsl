#version 330 core

// Night sky: dark gradient, moving storm clouds (fbm), hidden moon halo,
// and strong illumination of the clouds during the lightning flash.

in vec3 direction;
layout(location = 0) out vec4 FragColor;

uniform vec3 moon_direction;
uniform vec3 flash_direction;
uniform vec3 flash_color;
uniform float flash_intensity;
uniform vec3 fog_color;
uniform float time;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p)
{
	vec2 i = floor(p), f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(hash(i), hash(i + vec2(1, 0)), u.x), mix(hash(i + vec2(0, 1)), hash(i + vec2(1, 1)), u.x), u.y);
}
float fbm(vec2 p)
{
	float v = 0.0, a = 0.5;
	for (int k = 0; k < 6; ++k) { v += a * noise(p); p = p * 2.03 + vec2(1.7, 9.2); a *= 0.5; }
	return v;
}

void main()
{
	vec3 d = normalize(direction);
	float h = d.z;

	// Base gradient: very dark zenith, slightly brighter (light pollution) horizon
	vec3 zenith = vec3(0.012, 0.016, 0.032);
	vec3 horizon = vec3(0.075, 0.085, 0.12);
	vec3 col = mix(horizon, zenith, pow(clamp(h, 0.0, 1.0), 0.45));
	col += vec3(0.05, 0.03, 0.012) * exp(-max(h, 0.0) * 14.0); // orange city glow on the horizon

	// Clouds: fbm on the cloud plane, drifting with time
	float cloud = 0.0;
	if (h > -0.05) {
		vec2 uv = d.xy / (max(h, 0.0) + 0.12) * 1.2;
		uv += vec2(time * 0.035, time * 0.012);
		cloud = fbm(uv);
		cloud = smoothstep(0.35, 0.85, cloud);
	}

	// Moon: hidden behind the clouds, only a soft halo + a faint disk
	vec3 Lm = normalize(moon_direction);
	float cm = max(dot(d, Lm), 0.0);
	vec3 moon = vec3(0.55, 0.62, 0.8) * (pow(cm, 2000.0) * 1.5 * (1.0 - cloud) + pow(cm, 40.0) * 0.12 + pow(cm, 6.0) * 0.03);
	col += moon;
	col = mix(col, vec3(0.085, 0.095, 0.130) + 0.16 * pow(cm, 8.0) + vec3(0.03, 0.02, 0.01) * exp(-max(h, 0.0) * 6.0), cloud * 0.85);

	// Lightning: the whole cloud deck lights up, much more around the strike
	vec3 Lf = normalize(flash_direction);
	float cf = max(dot(d, Lf), 0.0);
	float around = 0.45 + 0.9 * pow(cf, 2.0) + 1.5 * pow(cf, 30.0);
	float density = 0.55 + 0.9 * cloud;
	col += flash_color * flash_intensity * around * density * (0.6 + 0.4 * smoothstep(-0.1, 0.4, h));

	// Below the horizon: blend into the fog color
	col = mix(col, fog_color + flash_color * flash_intensity * 0.30, smoothstep(0.02, -0.08, h));

	FragColor = vec4(col, 1.0);
}
