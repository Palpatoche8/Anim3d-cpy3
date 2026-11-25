#version 330 core

// Skyline buildings: same lighting as lit.frag, plus procedural windows.
// fragment.color carries data, not a color: (random seed, ratio of lit windows, kind)
//  kind 0: facade / roof    kind 1: street ground

in struct fragment_data
{
	vec3 position;
	vec3 normal;
	vec3 color;
	vec2 uv;
} fragment;

layout(location = 0) out vec4 FragColor;

struct phong_structure { float ambient; float diffuse; float specular; float specular_exponent; };
struct texture_settings_structure { bool use_texture; bool texture_inverse_v; bool two_sided; };
struct material_structure { vec3 color; float alpha; phong_structure phong; texture_settings_structure texture_settings; };
uniform material_structure material;

uniform vec3 camera_position;
uniform vec3 moon_direction;
uniform vec3 moon_color;
uniform vec3 ambient_color;
uniform vec3 flash_direction;
uniform vec3 flash_color;
uniform float flash_intensity;
uniform vec3 fog_color;
uniform float fog_density;
uniform float time;

float hash12(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

void main()
{
	vec3 N = normalize(fragment.normal);
	vec3 to_cam = camera_position - fragment.position;
	float dist = length(to_cam);
	vec3 V = to_cam / dist;

	float seed = fragment.color.x;
	float lit_ratio = fragment.color.y;
	float kind = fragment.color.z;

	vec3 albedo = material.color * (0.75 + 0.5 * seed);
	vec3 emission = vec3(0.0);
	float glass = 0.0;

	if (kind < 0.5) {
		if (abs(N.z) < 0.5) {
			// Window grid: one cell per floor (3.4m) and per bay (2.6m)
			vec2 cell_size = vec2(2.6, 3.4);
			vec2 g = fragment.uv / cell_size;
			vec2 cell = floor(g);
			vec2 local = fract(g);
			float in_window = step(0.18, local.x) * step(local.x, 0.82) * step(0.28, local.y) * step(local.y, 0.78);
			// Facade orientation in the hash so that each face differs
			vec2 id = cell + vec2(floor(seed * 1000.0), dot(N, vec3(17.0, 31.0, 0.0)));
			float h = hash12(id);
			// Whole floors tend to be lit/unlit together (offices)
			float floor_bias = hash12(vec2(cell.y, seed * 517.0)) - 0.5;
			bool lit = h < lit_ratio + 0.25 * floor_bias;
			if (in_window > 0.5) {
				glass = 1.0;
				if (lit) {
					float tint = hash12(id + 7.0);
					vec3 c = tint < 0.6 ? vec3(1.0, 0.72, 0.38) : (tint < 0.85 ? vec3(0.85, 0.88, 1.0) : vec3(0.45, 0.6, 1.0));
					float intensity = 0.35 + 0.65 * hash12(id + 3.0);
					// TV-like flicker on a few windows
					if (tint > 0.93) intensity *= 0.6 + 0.4 * sin(time * (6.0 + 10.0 * h) + h * 50.0);
					emission = c * intensity * 0.9;
				}
			}
		}
	}
	else {
		// Street level: dark wet asphalt + orange sodium lights along the streets
		albedo = vec3(0.03, 0.03, 0.035);
		vec2 m = mod(fragment.uv + 650.0, 26.0);
		float sx = exp(-pow(m.x - 22.8, 2.0) / 6.0);
		float sy = exp(-pow(m.y - 22.8, 2.0) / 6.0);
		emission = vec3(1.0, 0.52, 0.18) * 0.45 * max(sx, sy);
		glass = 0.6; // wet ground reflects the flash
	}

	// Lighting
	float up = 0.5 + 0.5 * N.z;
	vec3 ambient = mix(vec3(0.03, 0.02, 0.012), ambient_color, up) + flash_color * flash_intensity * 0.12;
	vec3 color = material.phong.ambient * ambient * albedo;

	vec3 Lm = normalize(moon_direction);
	color += moon_color * max(dot(N, Lm), 0.0) * albedo * material.phong.diffuse;

	vec3 Lf = normalize(flash_direction);
	float df = max(dot(N, Lf), 0.0);
	vec3 H = normalize(Lf + V);
	float spec = df > 0.0 ? pow(max(dot(N, H), 0.0), 40.0) : 0.0;
	color += flash_color * flash_intensity * (1.3 * df * albedo + (0.05 + 0.6 * glass) * spec);

	// Fog: the geometry fades in the haze, the lights pierce it a bit more
	float f = 1.0 - exp(-fog_density * dist);
	float f_light = 1.0 - exp(-0.55 * fog_density * dist);
	vec3 fog = fog_color + flash_color * flash_intensity * 0.30;
	fog += vec3(0.045, 0.028, 0.012) * clamp(-fragment.position.z / 60.0, 0.0, 1.0);
	color = mix(color, fog, f) + emission * (1.0 - f_light);

	FragColor = vec4(color, 1.0);
}
