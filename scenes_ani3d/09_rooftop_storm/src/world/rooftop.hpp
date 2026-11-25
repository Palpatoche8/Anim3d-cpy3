#pragma once

#include "cgp/cgp.hpp"
#include <vector>

// Axis aligned box used both for display and as a cape collider
struct collision_box {
	cgp::vec3 p_min, p_max;
};

// Rooftop of the building on which the character stands.
// Coordinates: z up, the roof top surface is at z = 0, the edge of the roof is at x = 0
// and the city lies toward +x (and all around, below).
struct rooftop_structure
{
	cgp::mesh_drawable drawable;          // all static rooftop geometry merged in one mesh
	std::vector<collision_box> colliders; // boxes the cape can collide with
	cgp::vec3 stand_point;                // where the character's feet are

	void initialize(cgp::opengl_shader_structure const& shader);
};
