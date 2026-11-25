#version 330 core

// Blinn-Phong lighting with:
//  - a cold and weak directional moon light
//  - the lightning flash (directional light toward the bolt + global ambient boost + rim light)
//  - hemispheric ambient (dark sky above, warm city glow from below)
//  - exponential distance fog (that lights up during the flash)

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

uniform vec3 emissive; // optional self illumination (reset to 0 by the environment)

vec3 blinn_phong(vec3 N, vec3 V, vec3 L, vec3 light_color, vec3 albedo, float kd, float ks, float shininess)
{
	float diffuse = max(dot(N, L), 0.0);
	vec3 H = normalize(L + V);
	float specular = diffuse > 0.0 ? pow(max(dot(N, H), 0.0), shininess) : 0.0;
	return light_color * (kd * diffuse * albedo + ks * specular);
}

void main()
{
	vec3 N = normalize(fragment.normal);
	if (material.texture_settings.two_sided && !gl_FrontFacing)
		N = -N;
	vec3 to_cam = camera_position - fragment.position;
	float dist = length(to_cam);
	vec3 V = to_cam / dist;

	vec3 albedo = fragment.color * material.color;
	float ka = material.phong.ambient, kd = material.phong.diffuse, ks = material.phong.specular;
	float shininess = material.phong.specular_exponent;

	// Hemispheric ambient: night sky from above, orange city light pollution from below
	float up = 0.5 + 0.5 * N.z;
	vec3 ambient = mix(vec3(0.030, 0.020, 0.012), ambient_color, up);
	ambient += flash_color * flash_intensity * 0.10;

	vec3 color = ka * ambient * albedo;
	color += blinn_phong(N, V, normalize(moon_direction), moon_color, albedo, kd, ks, shininess);
	color += blinn_phong(N, V, normalize(flash_direction), flash_color * flash_intensity * 1.4, albedo, kd, ks, shininess);

	// Rim light: outlines silhouettes against the sky (faint with the moon, strong with the flash)
	float rim = pow(1.0 - max(dot(N, V), 0.0), 6.0);
	color += rim * (0.10 * moon_color + 0.12 * flash_intensity * flash_color) * ks;

	color += emissive;

	// Distance fog (glows during the flash), denser and warmer near the street level
	float f = 1.0 - exp(-fog_density * dist);
	vec3 fog = fog_color + flash_color * flash_intensity * 0.30;
	fog += vec3(0.045, 0.028, 0.012) * clamp(-fragment.position.z / 60.0, 0.0, 1.0);
	color = mix(color, fog, f);

	FragColor = vec4(color, material.alpha);
}
