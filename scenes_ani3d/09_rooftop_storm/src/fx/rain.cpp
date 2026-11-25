#include "rain.hpp"

#include <algorithm>
#include <cmath>

using namespace cgp;

vec3 rain_structure::spawn(vec3 const& c, bool anywhere)
{
	std::uniform_real_distribution<float> U(-1.0f, 1.0f);
	float const z = anywhere ? c.z + 0.5f * height * U(rng) : c.z + 0.5f * height * (0.8f + 0.2f * U(rng));
	return {c.x + radius * U(rng), c.y + radius * U(rng), z};
}

void rain_structure::initialize(opengl_shader_structure const& shader_arg, vec3 const& camera_position)
{
	shader = shader_arg;
	std::uniform_real_distribution<float> U(0.0f, 1.0f);
	position.resize(max_drops);
	speed_factor.resize(max_drops);
	for (int k = 0; k < max_drops; ++k) {
		position[k] = spawn(camera_position, true);
		speed_factor[k] = 0.8f + 0.4f * U(rng);
	}
	// vertex = position (3) + alpha (1)
	buffer.initialize({3, 1});
}

static bool on_hero_roof(vec3 const& p)
{
	return p.x > -16.0f && p.x < 0.0f && p.y > -9.0f && p.y < 9.0f && p.z < 0.0f;
}

void rain_structure::update(float dt, vec3 const& wind, vec3 const& camera_position)
{
	vec3 const target_velocity = vec3{0, 0, -fall_speed} + wind_influence * vec3{wind.x, wind.y, 0.0f};
	// Drops follow the wind with some inertia (smooth tilt changes during gusts)
	float const a = std::min(1.0f, 3.0f * dt);
	current_velocity = (1 - a) * current_velocity + a * target_velocity;

	int const active = std::clamp(int(density * max_drops), 0, max_drops);
	vec3 const& c = camera_position;
	for (int k = 0; k < active; ++k) {
		vec3& p = position[k];
		p += dt * speed_factor[k] * current_velocity;

		// Keep the drops in the box around the camera: wrap horizontally, respawn at the top
		if (p.x < c.x - radius) p.x += 2 * radius;
		if (p.x > c.x + radius) p.x -= 2 * radius;
		if (p.y < c.y - radius) p.y += 2 * radius;
		if (p.y > c.y + radius) p.y -= 2 * radius;
		if (p.z < c.z - 0.5f * height || p.z > c.z + 0.6f * height || on_hero_roof(p))
			p = spawn(c, false);
	}

	// Build the streaks
	std::vector<float> data;
	data.reserve(active * 8);
	for (int k = 0; k < active; ++k) {
		vec3 const& head = position[k];
		vec3 const tail = head - streak * speed_factor[k] * current_velocity;
		data.insert(data.end(), {head.x, head.y, head.z, 1.0f, tail.x, tail.y, tail.z, 0.0f});
	}
	buffer.update(data);
}

void rain_structure::draw(environment_generic_structure const& environment)
{
	glUseProgram(shader.id);
	environment.send_opengl_uniform(shader, false);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_FALSE);
	buffer.draw(GL_LINES);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glUseProgram(0);
}
