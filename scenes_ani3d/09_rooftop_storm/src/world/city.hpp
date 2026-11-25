#pragma once

#include "cgp/cgp.hpp"

// Procedural skyline: hundreds of boxes of various sizes below and around the rooftop.
// All buildings are merged into a single mesh. The per-vertex 'color' attribute is used
// as data for the building shader: (random seed, ratio of lit windows, kind)
//   kind = 0 : building facade (windows drawn procedurally in the fragment shader)
//   kind = 1 : street level ground (sodium light glow)
struct city_structure
{
	cgp::mesh_drawable drawable;
	int building_count = 0;

	void initialize(cgp::opengl_shader_structure const& shader, unsigned int seed = 7);
};
