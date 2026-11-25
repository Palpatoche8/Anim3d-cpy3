#pragma once

#include "cgp/cgp.hpp"
#include "engine/gl_buffer.hpp"
#include <random>
#include <vector>

// Procedural lightning: bolt geometry (midpoint displacement + branches) and the
// flash envelope (strong peak then flickering decay over ~0.5s, with re-strokes).
struct lightning_structure
{
	struct segment { cgp::vec3 a, b; float intensity; float width; };

	// Parameters (GUI)
	bool automatic = true;
	float mean_interval = 7.0f;  // average time between two strikes (s)
	float flash_strength = 2.4f; // peak intensity of the flash light
	int subdivisions = 8;        // recursion depth of the midpoint displacement
	float branch_probability = 0.35f;

	// State
	std::vector<segment> segments;
	float time_since_strike = 1e3f;
	float next_strike_in = 2.5f;     // first strike soon after launch
	float flash = 0.0f;              // current flash intensity (output)
	float bolt_visibility = 0.0f;    // current bolt brightness (output)
	cgp::vec3 strike_position;       // middle of the bolt (for the flash direction)
	bool cloud_only = false;         // in-cloud flash, without visible bolt
	int strike_count = 0;

	std::mt19937 rng{1234};

	dynamic_vertex_buffer buffer;
	cgp::opengl_shader_structure shader;

	void initialize(cgp::opengl_shader_structure const& shader);
	void trigger(cgp::vec3 const& camera_position, cgp::vec3 const& camera_forward); // force a new strike
	void update(float dt, cgp::vec3 const& camera_position, cgp::vec3 const& camera_forward); // envelope + automatic triggering
	void draw(cgp::environment_generic_structure const& environment, cgp::vec3 const& camera_position);

private:
	struct stroke { float t0; float amplitude; };
	std::vector<stroke> strokes;
	float rand(float a = 0, float b = 1);
	void generate_bolt(cgp::vec3 const& start, cgp::vec3 const& end);
	void subdivide(cgp::vec3 const& a, cgp::vec3 const& b, float offset, int depth, float intensity, float width);
};
