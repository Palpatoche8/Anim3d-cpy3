#pragma once

#include "cgp/cgp.hpp"
#include <vector>

// Minimal dynamic vertex stream (VAO + one interleaved VBO of floats), updated every frame.
// Used for particles/lines where mesh_drawable would be overkill (rain, lightning bolt).
//   attribute_sizes = number of floats of each attribute, mapped to locations 0,1,2...
struct dynamic_vertex_buffer
{
	GLuint vao = 0;
	GLuint vbo = 0;
	int floats_per_vertex = 0;
	int vertex_count = 0;
	size_t capacity_bytes = 0;

	void initialize(std::vector<int> const& attribute_sizes);
	void update(std::vector<float> const& data); // re-uploads data (orphaning)
	void draw(GLenum primitive) const;
	void clear();
};
