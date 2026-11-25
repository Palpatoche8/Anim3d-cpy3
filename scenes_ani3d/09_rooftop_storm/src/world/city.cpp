#include "city.hpp"
#include "engine/mesh_builder.hpp"

#include <random>

using namespace cgp;

void city_structure::initialize(opengl_shader_structure const& shader, unsigned int seed)
{
	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> U(0.0f, 1.0f);

	float const street_z = -60.0f;  // street level (the hero rooftop is at z=0)
	float const block = 26.0f;      // city block size
	float const extent = 600.0f;

	mesh m;
	building_count = 0;

	auto add_building = [&](vec3 const& pmin, vec3 const& pmax, float lit_ratio) {
		mesh b = mesh_builder::box(pmin, pmax);
		vec3 const data = {U(rng), lit_ratio, 0.0f};
		mesh_builder::paint(b, data);
		m.push_back(b);
		building_count++;
	};

	for (float bx = -extent; bx < extent; bx += block) {
		for (float by = -extent; by < extent; by += block) {
			// Each block is split in 1 to 4 lots
			int const nx = 1 + int(U(rng) * 2.0f), ny = 1 + int(U(rng) * 2.0f);
			float const street = 7.0f;
			float const lx = (block - street) / nx, ly = (block - street) / ny;
			for (int i = 0; i < nx; ++i) {
				for (int j = 0; j < ny; ++j) {
					float const x0 = bx + i * lx + 0.6f, y0 = by + j * ly + 0.6f;
					float const x1 = x0 + lx * (0.75f + 0.25f * U(rng)) - 0.6f;
					float const y1 = y0 + ly * (0.75f + 0.25f * U(rng)) - 0.6f;

					// Keep the area of the hero building (and its immediate surroundings) free
					if (x1 > -34.0f && x0 < 18.0f && y1 > -26.0f && y0 < 26.0f)
						continue;
					vec2 const c = {0.5f * (x0 + x1), 0.5f * (y0 + y1)};
					float const d = norm(c);
					if (d > extent)
						continue;

					// Height: below the hero rooftop nearby (we look down on them),
					// taller towers far away to build the skyline on the horizon.
					float const far = std::clamp((d - 60.0f) / 400.0f, 0.0f, 1.0f);
					float h = 12.0f + 34.0f * U(rng) + far * 60.0f * U(rng) * U(rng);
					if (d > 160.0f && U(rng) < 0.04f + 0.12f * far) h += 40.0f + 90.0f * U(rng) * far; // skyscraper
					float const lit = 0.10f + 0.35f * U(rng);

					add_building({x0, y0, street_z}, {x1, y1, street_z + h}, lit);

					// Setback / roof structure on top of tall buildings
					if (h > 55.0f && U(rng) < 0.7f) {
						float const sx = 0.15f + 0.15f * U(rng), sy = 0.15f + 0.15f * U(rng);
						float const w = x1 - x0, l = y1 - y0;
						float const h2 = 6.0f + 25.0f * U(rng);
						add_building({x0 + sx * w, y0 + sy * l, street_z + h}, {x1 - sx * w, y1 - sy * l, street_z + h + h2}, lit);
						if (U(rng) < 0.5f) // spire
							add_building({0.5f * (x0 + x1) - 0.4f, 0.5f * (y0 + y1) - 0.4f, street_z + h + h2}, {0.5f * (x0 + x1) + 0.4f, 0.5f * (y0 + y1) + 0.4f, street_z + h + h2 + 8.0f + 12.0f * U(rng)}, 0.0f);
					}
				}
			}
		}
	}

	// Street level ground
	{
		float const E = extent * 1.6f;
		mesh g = mesh_primitive_quadrangle({-E, -E, street_z}, {E, -E, street_z}, {E, E, street_z}, {-E, E, street_z});
		g.uv = {{-E, -E}, {E, -E}, {E, E}, {-E, E}}; // world coordinates (used by the shader for the street grid)
		mesh_builder::paint(g, {0.0f, 0.0f, 1.0f});
		m.push_back(g);
	}

	drawable.initialize_data_on_gpu(m, shader);
	drawable.material.color = {0.085f, 0.095f, 0.12f};
	drawable.material.phong.specular = 0.05f;
}
