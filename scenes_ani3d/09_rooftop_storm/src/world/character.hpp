#pragma once

#include "cgp/cgp.hpp"
#include <vector>

// Capsule = segment [a,b] inflated by radius r (a sphere when a == b)
struct capsule {
	cgp::vec3 a, b;
	float r;
};

// Original hooded vigilante: static, low-poly silhouette built from simple primitives.
// Faces +x (toward the city), z is up. Its feet are at 'origin'.
// It exposes collision capsules and the attachment curve of the cape (across the shoulders).
struct character_structure
{
	cgp::mesh_drawable body;   // dark suit, hood, belt
	cgp::mesh_drawable eyes;   // faint glowing eyes (emissive)
	std::vector<capsule> colliders;
	cgp::vec3 origin;

	void initialize(cgp::vec3 const& feet_position, cgp::opengl_shader_structure const& shader);

	// Attachment point of the cape on the shoulders, s in [-1,1] goes from the right shoulder to the left one
	cgp::vec3 cape_anchor(float s) const;
};
