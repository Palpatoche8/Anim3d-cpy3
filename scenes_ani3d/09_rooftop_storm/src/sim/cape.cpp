#include "cape.hpp"

#include <algorithm>

using namespace cgp;

void cape_structure::initialize(character_structure const& character, opengl_shader_structure const& shader)
{
	int const nu = param.nu, nv = param.nv;
	int const N = nu * nv;
	x.assign(N, {0, 0, 0});
	v.assign(N, {0, 0, 0});
	w.assign(N, 1.0f);
	diverged = false;
	accumulated_time = 0.0f;

	// Initial (= rest) shape: trapezoid hanging behind the back, from the shoulder arc
	// to a wider and slightly curved hem.
	vec3 const o = character.origin;
	for (int j = 0; j < nv; ++j) {
		float const t = j / float(nv - 1);
		for (int i = 0; i < nu; ++i) {
			float const s = -1.0f + 2.0f * i / float(nu - 1);
			vec3 const top = character.cape_anchor(s);
			float const c = 1.0f - s * s;
			vec3 const bottom = {o.x - 0.30f - 0.12f * c, o.y + 0.5f * param.width_bottom * s, top.z - param.length};
			// Slight bulge so the cape starts outside the body colliders
			vec3 pos = (1 - t) * top + t * bottom;
			pos.x -= 0.12f * std::sin(Pi * t) * (0.3f + 0.7f * c);
			x[index(i, j)] = pos;
		}
	}
	float const particle_mass = param.mass / float(N);
	for (int i = 0; i < nu; ++i)
		w[index(i, 0)] = 0.0f; // top row pinned
	for (int k = 0; k < N; ++k)
		if (w[k] > 0) w[k] = 1.0f / particle_mass;
	p = x;

	// Constraints: rest lengths taken from the initial shape
	constraints.clear();
	auto add = [&](int i0, int j0, int i1, int j1, int type) {
		if (i1 < 0 || i1 >= nu || j1 < 0 || j1 >= nv) return;
		int const a = index(i0, j0), b = index(i1, j1);
		constraints.push_back({a, b, norm(x[a] - x[b]), type});
	};
	for (int j = 0; j < nv; ++j) {
		for (int i = 0; i < nu; ++i) {
			add(i, j, i + 1, j, 0);     // structural
			add(i, j, i, j + 1, 0);
			add(i, j, i + 1, j + 1, 1); // shear
			add(i + 1, j, i, j + 1, 1);
			add(i, j, i + 2, j, 2);     // bending
			add(i, j, i, j + 2, 2);
		}
	}
	// Structural first, then shear, then bending (better convergence of the important ones)
	std::stable_sort(constraints.begin(), constraints.end(), [](auto const& a, auto const& b) { return a.type < b.type; });

	// Rendering mesh (uv: u across, v along the length)
	mesh m;
	for (int j = 0; j < nv; ++j)
		for (int i = 0; i < nu; ++i) {
			m.position.push_back(x[index(i, j)]);
			m.uv.push_back({i / float(nu - 1), j / float(nv - 1)});
		}
	for (int j = 0; j < nv - 1; ++j)
		for (int i = 0; i < nu - 1; ++i) {
			unsigned const a = index(i, j), b = index(i + 1, j), c = index(i + 1, j + 1), d = index(i, j + 1);
			m.connectivity.push_back({a, c, b});
			m.connectivity.push_back({a, d, c});
		}
	m.normal = normal_per_vertex(m.position, m.connectivity);
	m.fill_empty_field();
	triangles = m.connectivity;
	position_buffer = m.position;
	normal_buffer = m.normal;

	drawable.clear();
	drawable.initialize_data_on_gpu(m, shader);
	drawable.material.color = {0.035f, 0.037f, 0.05f};
	drawable.material.phong.ambient = 1.0f;
	drawable.material.phong.diffuse = 1.0f;
	drawable.material.phong.specular = 0.25f;
	drawable.material.phong.specular_exponent = 18.0f;
	drawable.material.texture_settings.two_sided = true;
}

void cape_structure::step(float frame_dt, wind_structure const& wind, character_structure const& character, rooftop_structure const& roof)
{
	if (diverged)
		return;
	// Fixed time step for stability, independent of the frame rate
	float const h = 1.0f / (60.0f * param.substeps);
	accumulated_time += std::min(frame_dt, 0.05f);
	int count = 0;
	while (accumulated_time >= h && count < 4 * param.substeps) {
		substep(h, wind, character, roof);
		accumulated_time -= h;
		count++;
	}
	if (count == 4 * param.substeps)
		accumulated_time = 0.0f; // too slow: drop time rather than spiral down

	for (auto const& q : x)
		if (!(std::abs(q.x) < 1e4f && std::abs(q.y) < 1e4f && std::abs(q.z) < 1e4f))
			diverged = true;
}

void cape_structure::substep(float h, wind_structure const& wind, character_structure const& character, rooftop_structure const& roof)
{
	int const N = int(x.size());
	int const nu = param.nu, nv = param.nv;
	std::vector<vec3> force(N, vec3{0, 0, 0});

	// 1. Aerodynamic forces per triangle (pressure along the normal ~ |v_n| v_n, plus friction)
	float const rho_half = 0.6f; // 0.5 * air density
	for (int j = 0; j < nv - 1; ++j) {
		for (int i = 0; i < nu - 1; ++i) {
			int const quad[2][3] = {
				{index(i, j), index(i + 1, j + 1), index(i + 1, j)},
				{index(i, j), index(i, j + 1), index(i + 1, j + 1)} };
			for (auto const& tri : quad) {
				vec3 const& a = x[tri[0]]; vec3 const& b = x[tri[1]]; vec3 const& c = x[tri[2]];
				vec3 const n2 = cross(b - a, c - a);
				float const n2_norm = norm(n2);
				if (n2_norm < 1e-8f) continue;
				float const area = 0.5f * n2_norm;
				vec3 const n = n2 / n2_norm;
				vec3 const center = (a + b + c) / 3.0f;
				vec3 const vel = (v[tri[0]] + v[tri[1]] + v[tri[2]]) / 3.0f;
				vec3 const vrel = wind.at(center) - vel;
				float const vn = dot(vrel, n);
				vec3 const vt = vrel - vn * n;
				vec3 const f = rho_half * area * (param.drag_normal * std::abs(vn) * vn * n + param.drag_tangent * norm(vt) * vt);
				for (int k = 0; k < 3; ++k)
					force[tri[k]] += f / 3.0f;
			}
		}
	}

	// 2. Explicit prediction (symplectic Euler) with damping
	vec3 const g = {0, 0, -param.gravity};
	float const damp = std::max(0.0f, 1.0f - param.damping * h);
	for (int k = 0; k < N; ++k) {
		if (w[k] == 0.0f) {
			p[k] = x[k];
			continue;
		}
		v[k] = damp * (v[k] + h * (g + w[k] * force[k]));
		p[k] = x[k] + h * v[k];
	}

	// 3. Constraint projection (Gauss-Seidel), stiffness corrected for the iteration count
	int const iters = std::max(1, param.iterations);
	float const k_eff[3] = {
		1.0f - std::pow(1.0f - std::clamp(param.stiffness_structural, 0.0f, 0.999f), 1.0f / iters),
		1.0f - std::pow(1.0f - std::clamp(param.stiffness_shear, 0.0f, 0.999f), 1.0f / iters),
		1.0f - std::pow(1.0f - std::clamp(param.stiffness_bend, 0.0f, 0.999f), 1.0f / iters) };

	for (int it = 0; it < iters; ++it) {
		for (auto const& c : constraints) {
			float const wsum = w[c.i] + w[c.j];
			if (wsum == 0.0f) continue;
			vec3 const d = p[c.i] - p[c.j];
			float const L = norm(d);
			if (L < 1e-7f) continue;
			// Bending: only resist compression lightly and stretching fully (allows nice folds)
			float C = L - c.rest;
			if (c.type == 2 && C < 0) C *= 0.5f;
			vec3 const corr = (k_eff[c.type] * C / (wsum * L)) * d;
			p[c.i] -= w[c.i] * corr;
			p[c.j] += w[c.j] * corr;
		}
		solve_collisions(character, roof);
	}

	// 4. Velocity update
	float const inv_h = 1.0f / h;
	for (int k = 0; k < N; ++k) {
		v[k] = (p[k] - x[k]) * inv_h;
		x[k] = p[k];
	}
}

void cape_structure::solve_collisions(character_structure const& character, rooftop_structure const& roof)
{
	float const th = param.thickness;
	int const N = int(p.size());
	for (int k = 0; k < N; ++k) {
		if (w[k] == 0.0f) continue;
		vec3& q = p[k];

		// Body capsules: project outside of the inflated segment
		for (capsule const& c : character.colliders) {
			vec3 const ab = c.b - c.a;
			float const L2 = dot(ab, ab);
			float t = L2 > 1e-8f ? std::clamp(dot(q - c.a, ab) / L2, 0.0f, 1.0f) : 0.0f;
			vec3 const closest = c.a + t * ab;
			vec3 const d = q - closest;
			float const dist = norm(d);
			float const R = c.r + th;
			if (dist < R) {
				vec3 const n = dist > 1e-6f ? d / dist : vec3{-1, 0, 0};
				q = closest + R * n;
			}
		}

		// Rooftop boxes: push out along the axis of smallest penetration
		for (collision_box const& b : roof.colliders) {
			vec3 const lo = b.p_min - vec3{th, th, th};
			vec3 const hi = b.p_max + vec3{th, th, th};
			if (q.x > lo.x && q.x < hi.x && q.y > lo.y && q.y < hi.y && q.z > lo.z && q.z < hi.z) {
				float const pen[6] = { q.x - lo.x, hi.x - q.x, q.y - lo.y, hi.y - q.y, q.z - lo.z, hi.z - q.z };
				int best = 0;
				for (int a = 1; a < 6; ++a)
					if (pen[a] < pen[best]) best = a;
				switch (best) {
				case 0: q.x = lo.x; break; case 1: q.x = hi.x; break;
				case 2: q.y = lo.y; break; case 3: q.y = hi.y; break;
				case 4: q.z = lo.z; break; case 5: q.z = hi.z; break;
				}
			}
		}
	}
}

void cape_structure::update_drawable()
{
	for (size_t k = 0; k < x.size(); ++k)
		position_buffer[k] = x[k];
	normal_per_vertex(position_buffer, triangles, normal_buffer);
	drawable.vbo_position.update(position_buffer);
	drawable.vbo_normal.update(normal_buffer);
}
