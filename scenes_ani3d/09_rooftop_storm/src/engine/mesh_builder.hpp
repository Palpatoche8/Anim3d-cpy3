#pragma once

#include "cgp/cgp.hpp"

// Small procedural geometry helpers built on top of cgp::mesh.
// Every function returns a mesh with normals, uv and a uniform per-vertex color.
namespace mesh_builder
{
	// Axis aligned box with flat faces. uv = metric coordinates on each face (useful for procedural windows)
	cgp::mesh box(cgp::vec3 const& p_min, cgp::vec3 const& p_max, cgp::vec3 const& color = {1, 1, 1});

	// Tube between p0 and p1 with elliptic cross sections (r0 at p0, r1 at p1).
	// 'side' gives the direction of the first ellipse axis (rx), the other axis is perpendicular.
	cgp::mesh tapered_tube(cgp::vec3 const& p0, cgp::vec3 const& p1, cgp::vec2 const& r0, cgp::vec2 const& r1,
		cgp::vec3 const& side, cgp::vec3 const& color, int N_around = 16, int N_length = 4, bool caps = true);

	// Ellipsoid with semi-axes 'radius' expressed in the frame (ex, ey, ez = cross(ex,ey))
	cgp::mesh ellipsoid(cgp::vec3 const& center, cgp::vec3 const& radius, cgp::vec3 const& ex, cgp::vec3 const& ey,
		cgp::vec3 const& color, int Nu = 24, int Nv = 14);

	// Cone from base center toward apex with elliptic base
	cgp::mesh cone(cgp::vec3 const& base, cgp::vec3 const& apex, cgp::vec2 const& radius, cgp::vec3 const& side,
		cgp::vec3 const& color, int N = 16);

	// Set the color of all vertices
	void paint(cgp::mesh& m, cgp::vec3 const& color);
}
