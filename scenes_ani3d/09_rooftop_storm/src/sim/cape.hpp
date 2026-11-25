#pragma once

#include "cgp/cgp.hpp"
#include "sim/wind.hpp"
#include "world/character.hpp"
#include "world/rooftop.hpp"
#include <vector>

struct cape_parameters
{
	// Grid resolution (nu across the shoulders, nv along the length)
	int nu = 22;
	int nv = 30;
	// Shape (meters)
	float length = 1.50f;
	float width_bottom = 1.70f;

	// Position Based Dynamics
	float stiffness_structural = 1.0f; // in [0,1]
	float stiffness_shear = 0.85f;
	float stiffness_bend = 0.10f;
	int iterations = 12;
	int substeps = 4;
	float damping = 0.6f;     // linear velocity damping (1/s)
	float mass = 1.4f;        // total mass of the cape (kg)
	float gravity = 9.81f;

	// Aerodynamics
	float drag_normal = 1.0f;     // pressure coefficient (normal to the cloth)
	float drag_tangent = 0.05f;   // friction coefficient (tangential)
	float thickness = 0.025f;     // collision margin
};

// Cape simulated as a grid of particles with PBD distance constraints
//  (structural + shear + bending), wind aerodynamic forces and body collisions.
struct cape_structure
{
	struct distance_constraint { int i, j; float rest; int type; }; // type: 0 structural, 1 shear, 2 bend

	cape_parameters param;

	std::vector<cgp::vec3> x;      // positions
	std::vector<cgp::vec3> p;      // predicted positions (PBD)
	std::vector<cgp::vec3> v;      // velocities
	std::vector<float> w;          // inverse mass (0 = pinned to the shoulders)
	std::vector<distance_constraint> constraints;

	cgp::numarray<cgp::vec3> position_buffer; // for rendering / normals
	cgp::numarray<cgp::vec3> normal_buffer;
	cgp::numarray<cgp::uint3> triangles;
	cgp::mesh_drawable drawable;

	bool diverged = false;
	float accumulated_time = 0.0f;

	int index(int i, int j) const { return j * param.nu + i; } // i across, j along the length

	void initialize(character_structure const& character, cgp::opengl_shader_structure const& shader);
	void step(float frame_dt, wind_structure const& wind, character_structure const& character, rooftop_structure const& roof);
	void update_drawable(); // recompute normals and upload to GPU

private:
	void substep(float h, wind_structure const& wind, character_structure const& character, rooftop_structure const& roof);
	void solve_collisions(character_structure const& character, rooftop_structure const& roof);
};
