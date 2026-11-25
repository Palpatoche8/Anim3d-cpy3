#pragma once

#include "cgp/cgp.hpp"
#include "engine/gl_buffer.hpp"
#include <random>
#include <vector>

// Rain: CPU particle system in a box that follows the camera.
// Drops fall with gravity-like terminal speed + a fraction of the wind (tilted streaks),
// and are rendered as short lines along their velocity.
struct rain_structure
{
	int max_drops = 30000;
	float density = 0.6f;      // fraction of max_drops that are active
	float fall_speed = 11.0f;  // terminal velocity (m/s)
	float wind_influence = 0.8f;
	float streak = 0.045f;     // streak length as a duration (s): line = velocity * streak
	float radius = 22.0f;      // half size of the box around the camera
	float height = 28.0f;

	std::vector<cgp::vec3> position;
	std::vector<float> speed_factor;

	dynamic_vertex_buffer buffer;
	cgp::opengl_shader_structure shader;
	std::mt19937 rng{42};

	void initialize(cgp::opengl_shader_structure const& shader, cgp::vec3 const& camera_position);
	void update(float dt, cgp::vec3 const& wind, cgp::vec3 const& camera_position);
	void draw(cgp::environment_generic_structure const& environment);

private:
	cgp::vec3 current_velocity{0, 0, -11.0f};
	cgp::vec3 spawn(cgp::vec3 const& camera_position, bool anywhere);
};
