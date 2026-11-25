#include "rooftop.hpp"
#include "engine/mesh_builder.hpp"

using namespace cgp;
using mesh_builder::box;

void rooftop_structure::initialize(opengl_shader_structure const& shader)
{
	mesh m;
	vec3 const concrete = {0.30f, 0.31f, 0.34f};
	vec3 const dark_concrete = {0.20f, 0.21f, 0.24f};
	vec3 const metal = {0.25f, 0.27f, 0.30f};

	float const roof_x0 = -16.0f, roof_x1 = 0.0f;
	float const roof_y0 = -9.0f, roof_y1 = 9.0f;

	// Main building body (extends down to the street, 60m below)
	m.push_back(box({roof_x0, roof_y0, -60.0f}, {roof_x1, roof_y1, -0.25f}, dark_concrete));
	// Roof slab
	m.push_back(box({roof_x0, roof_y0, -0.25f}, {roof_x1, roof_y1, 0.0f}, concrete));

	// Parapet (low wall all around the roof)
	float const ph = 0.75f, pt = 0.35f;
	m.push_back(box({roof_x1 - pt, roof_y0, 0}, {roof_x1, roof_y1, ph}, concrete));         // front edge (city side)
	m.push_back(box({roof_x0, roof_y0, 0}, {roof_x0 + pt, roof_y1, ph}, concrete));         // back
	m.push_back(box({roof_x0, roof_y0, 0}, {roof_x1, roof_y0 + pt, ph}, concrete));         // left
	m.push_back(box({roof_x0, roof_y1 - pt, 0}, {roof_x1, roof_y1, ph}, concrete));         // right
	// Coping stones on the front parapet (slightly overhanging)
	m.push_back(box({roof_x1 - pt - 0.05f, roof_y0, ph}, {roof_x1 + 0.08f, roof_y1, ph + 0.08f}, dark_concrete));

	// Cornice running along the facade just below the roof
	m.push_back(box({roof_x1, roof_y0, -1.2f}, {roof_x1 + 0.35f, roof_y1, -0.8f}, dark_concrete));

	// Stylized gargoyle block at the corner of the ledge: the character stands on it
	//  base pedestal + crouching beast silhouette (blocks + wings)
	vec3 const g = {0.0f, 0.0f, ph + 0.08f};
	m.push_back(box({g.x - 0.45f, g.y - 0.45f, g.z}, {g.x + 0.55f, g.y + 0.45f, g.z + 0.35f}, dark_concrete)); // pedestal
	m.push_back(box({g.x - 0.40f, g.y - 0.38f, g.z + 0.35f}, {g.x + 0.75f, g.y + 0.38f, g.z + 0.55f}, concrete)); // body slab (stand area)
	// Beast head protruding over the void
	m.push_back(mesh_builder::ellipsoid({g.x + 0.95f, g.y, g.z + 0.42f}, {0.30f, 0.22f, 0.20f}, {1, 0, 0}, {0, 1, 0}, concrete, 16, 10));
	m.push_back(mesh_builder::cone({g.x + 1.15f, g.y, g.z + 0.42f}, {g.x + 1.45f, g.y, g.z + 0.32f}, {0.12f, 0.08f}, {0, 1, 0}, dark_concrete, 10)); // snout
	m.push_back(mesh_builder::cone({g.x + 0.95f, g.y - 0.12f, g.z + 0.56f}, {g.x + 0.82f, g.y - 0.20f, g.z + 0.82f}, {0.05f, 0.05f}, {1, 0, 0}, dark_concrete, 8)); // horns
	m.push_back(mesh_builder::cone({g.x + 0.95f, g.y + 0.12f, g.z + 0.56f}, {g.x + 0.82f, g.y + 0.20f, g.z + 0.82f}, {0.05f, 0.05f}, {1, 0, 0}, dark_concrete, 8));
	// Folded stone wings on each side
	for (int s = -1; s <= 1; s += 2) {
		m.push_back(mesh_builder::tapered_tube({g.x - 0.2f, g.y + s * 0.40f, g.z + 0.45f}, {g.x + 0.7f, g.y + s * 0.48f, g.z + 0.25f},
			{0.06f, 0.22f}, {0.03f, 0.08f}, {0, 0, 1}, dark_concrete, 10, 2));
	}
	stand_point = {g.x + 0.10f, g.y, g.z + 0.55f};

	// Roof clutter: water tower, AC units, stairwell housing, antenna
	vec3 const tank_c = {-11.0f, 5.0f, 0.0f};
	for (int i = 0; i < 4; ++i) {
		float const a = Pi / 4 + i * Pi / 2;
		vec3 const leg = tank_c + vec3{1.1f * std::cos(a), 1.1f * std::sin(a), 0};
		m.push_back(mesh_builder::tapered_tube(leg, leg + vec3{0, 0, 3.0f}, {0.08f, 0.08f}, {0.08f, 0.08f}, {1, 0, 0}, metal, 6, 1));
	}
	m.push_back(mesh_builder::tapered_tube(tank_c + vec3{0, 0, 3.0f}, tank_c + vec3{0, 0, 6.0f}, {1.5f, 1.5f}, {1.5f, 1.5f}, {1, 0, 0}, {0.22f, 0.17f, 0.14f}, 24, 3));
	m.push_back(mesh_builder::cone(tank_c + vec3{0, 0, 6.0f}, tank_c + vec3{0, 0, 7.1f}, {1.6f, 1.6f}, {1, 0, 0}, metal, 24));

	m.push_back(box({-9.0f, -7.5f, 0}, {-5.0f, -4.0f, 3.2f}, dark_concrete)); // stairwell
	m.push_back(box({-9.2f, -7.7f, 3.2f}, {-4.8f, -3.8f, 3.4f}, concrete));
	m.push_back(box({-5.05f, -6.2f, 0}, {-4.95f, -5.0f, 2.2f}, {0.15f, 0.12f, 0.10f})); // door

	m.push_back(box({-4.0f, 4.0f, 0}, {-2.2f, 5.4f, 1.1f}, metal)); // AC units
	m.push_back(box({-4.0f, 6.0f, 0}, {-2.2f, 7.4f, 1.1f}, metal));
	m.push_back(mesh_builder::tapered_tube({-13.0f, -6.5f, 0}, {-13.0f, -6.5f, 9.0f}, {0.07f, 0.07f}, {0.03f, 0.03f}, {1, 0, 0}, metal, 6, 2));

	m.fill_empty_field();
	drawable.initialize_data_on_gpu(m, shader);
	drawable.material.phong.specular = 0.15f;
	drawable.material.phong.specular_exponent = 24.0f;

	// Colliders for the cape (roof floor, parapet, gargoyle)
	colliders.push_back({{roof_x0, roof_y0, -1.0f}, {roof_x1, roof_y1, 0.0f}});
	colliders.push_back({{roof_x1 - pt - 0.05f, roof_y0, 0.0f}, {roof_x1 + 0.08f, roof_y1, ph + 0.08f}});
	colliders.push_back({{g.x - 0.45f, g.y - 0.45f, g.z}, {g.x + 0.75f, g.y + 0.45f, g.z + 0.55f}});
}
