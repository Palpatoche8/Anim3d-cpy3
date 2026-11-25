#pragma once

#include "cgp/cgp.hpp"

// Simple camera with two modes (z is the up axis):
//  - Orbit : rotates around a target point (mouse left = rotate, right = zoom,
//            ctrl+left = pan, scroll = zoom)
//  - Free  : fly camera (WASD = ZQSD on AZERTY, E/C up/down, mouse left = look)
// Switch with the TAB key or in the GUI.
struct camera_rig
{
	enum class mode_type { orbit, free };
	mode_type mode = mode_type::orbit;

	// Orbit parameters
	cgp::vec3 target = {0, 0, 1.6f};
	float distance = 6.0f;
	// Shared orientation (radians): yaw around z, pitch above the horizon
	float yaw = 0.0f;
	float pitch = 0.0f;

	// Free mode position
	cgp::vec3 free_position = {0, 0, 0};
	float free_speed = 6.0f;

	void look_at(cgp::vec3 const& eye, cgp::vec3 const& center);
	void set_mode(mode_type m);

	cgp::vec3 forward() const; // unit view direction
	cgp::vec3 position() const;
	cgp::mat4 matrix_view() const;

	// Input handling (called from the scene event functions)
	void mouse_move(cgp::input_devices const& inputs);
	void mouse_scroll(float scroll);
	void update(cgp::input_devices const& inputs, float dt); // continuous keyboard motion
};
