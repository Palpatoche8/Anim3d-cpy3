#pragma once

#include "cgp/cgp.hpp"

using namespace cgp;

// ********************************************************************* //
// Global lighting / atmosphere state shared by every shader of the scene.
// Sent automatically by CGP at each draw() call.
// ********************************************************************* //
struct environment_structure : environment_generic_structure
{
	vec3 background_color = {0.01f, 0.012f, 0.02f};

	mat4 camera_view;
	mat4 camera_projection;
	vec3 camera_position;

	// Cold, weak moon light (directional)
	vec3 moon_direction = normalize(vec3{-0.35f, 0.45f, 0.82f}); // direction TOWARD the moon
	vec3 moon_color = {0.20f, 0.26f, 0.42f};
	vec3 ambient_color = {0.020f, 0.026f, 0.045f};

	// Lightning light (directional from the bolt + global ambient boost)
	vec3 flash_direction = {1, 0, 0.3f}; // direction TOWARD the bolt
	vec3 flash_color = {0.80f, 0.86f, 1.0f};
	float flash_intensity = 0.0f;         // 0 = no flash, >1 = blinding

	// Distance fog
	vec3 fog_color = {0.030f, 0.038f, 0.065f};
	float fog_density = 0.0065f;

	float time = 0.0f;

	void send_opengl_uniform(opengl_shader_structure const& shader, bool expected = default_expected_uniform) const override;
};


// Global variables storing general information on the project
struct project {
	static std::string path;
	static float gui_scale;
	static bool fps_limiting;
	static float fps_max;
	static bool vsync;
	static float initial_window_size_width;
	static float initial_window_size_height;
};
