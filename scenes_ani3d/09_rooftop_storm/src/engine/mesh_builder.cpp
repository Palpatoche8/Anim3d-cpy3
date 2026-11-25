#include "mesh_builder.hpp"

using namespace cgp;

namespace mesh_builder
{
	void paint(mesh& m, vec3 const& color)
	{
		m.color.resize(m.position.size());
		for (auto& c : m.color)
			c = color;
	}

	// Orthonormal frame (u, v) perpendicular to axis, with u as close as possible to 'side'
	static void perpendicular_frame(vec3 const& axis, vec3 const& side, vec3& u, vec3& v)
	{
		vec3 s = side - dot(side, axis) * axis;
		if (norm(s) < 1e-5f) {
			s = std::abs(axis.z) < 0.9f ? vec3{0, 0, 1} : vec3{1, 0, 0};
			s = s - dot(s, axis) * axis;
		}
		u = normalize(s);
		v = cross(axis, u);
	}

	mesh box(vec3 const& a, vec3 const& b, vec3 const& color)
	{
		mesh m;
		auto quad = [&](vec3 const& p00, vec3 const& p10, vec3 const& p11, vec3 const& p01, vec3 const& n) {
			unsigned int const k = unsigned(m.position.size());
			m.position.push_back(p00); m.position.push_back(p10);
			m.position.push_back(p11); m.position.push_back(p01);
			for (int i = 0; i < 4; ++i) m.normal.push_back(n);
			float const w = norm(p10 - p00), h = norm(p01 - p00);
			m.uv.push_back({0, 0}); m.uv.push_back({w, 0});
			m.uv.push_back({w, h}); m.uv.push_back({0, h});
			m.connectivity.push_back({k, k + 1, k + 2});
			m.connectivity.push_back({k, k + 2, k + 3});
		};
		// Side faces: uv.x along the facade, uv.y = height (in meters)
		quad({a.x, a.y, a.z}, {b.x, a.y, a.z}, {b.x, a.y, b.z}, {a.x, a.y, b.z}, {0, -1, 0});
		quad({b.x, b.y, a.z}, {a.x, b.y, a.z}, {a.x, b.y, b.z}, {b.x, b.y, b.z}, {0, 1, 0});
		quad({a.x, b.y, a.z}, {a.x, a.y, a.z}, {a.x, a.y, b.z}, {a.x, b.y, b.z}, {-1, 0, 0});
		quad({b.x, a.y, a.z}, {b.x, b.y, a.z}, {b.x, b.y, b.z}, {b.x, a.y, b.z}, {1, 0, 0});
		// Top and bottom
		quad({a.x, a.y, b.z}, {b.x, a.y, b.z}, {b.x, b.y, b.z}, {a.x, b.y, b.z}, {0, 0, 1});
		quad({a.x, b.y, a.z}, {b.x, b.y, a.z}, {b.x, a.y, a.z}, {a.x, a.y, a.z}, {0, 0, -1});
		paint(m, color);
		return m;
	}

	mesh tapered_tube(vec3 const& p0, vec3 const& p1, vec2 const& r0, vec2 const& r1, vec3 const& side,
		vec3 const& color, int N_around, int N_length, bool caps)
	{
		mesh m;
		vec3 const axis = normalize(p1 - p0);
		float const L = norm(p1 - p0);
		vec3 u, v;
		perpendicular_frame(axis, side, u, v);

		for (int j = 0; j <= N_length; ++j) {
			float const t = j / float(N_length);
			vec2 const r = (1 - t) * r0 + t * r1;
			vec3 const c = p0 + t * L * axis;
			for (int i = 0; i <= N_around; ++i) {
				float const a = 2 * Pi * i / float(N_around);
				float const ca = std::cos(a), sa = std::sin(a);
				m.position.push_back(c + r.x * ca * u + r.y * sa * v);
				// Normal of an elliptic cone (approximation, good enough for a stylized body)
				vec3 n = (ca / std::max(r.x, 1e-4f)) * u + (sa / std::max(r.y, 1e-4f)) * v;
				float const slope = ((r0.x + r0.y) - (r1.x + r1.y)) * 0.5f / std::max(L, 1e-4f);
				n = normalize(normalize(n) + slope * axis);
				m.normal.push_back(n);
				m.uv.push_back({i / float(N_around), t});
			}
		}
		for (int j = 0; j < N_length; ++j) {
			for (int i = 0; i < N_around; ++i) {
				unsigned const k00 = j * (N_around + 1) + i, k10 = k00 + 1;
				unsigned const k01 = k00 + (N_around + 1), k11 = k01 + 1;
				m.connectivity.push_back({k00, k10, k11});
				m.connectivity.push_back({k00, k11, k01});
			}
		}
		if (caps) {
			for (int end = 0; end < 2; ++end) {
				vec3 const c = end == 0 ? p0 : p1;
				vec2 const r = end == 0 ? r0 : r1;
				vec3 const n = end == 0 ? -axis : axis;
				unsigned const kc = unsigned(m.position.size());
				m.position.push_back(c); m.normal.push_back(n); m.uv.push_back({0.5f, 0.5f});
				for (int i = 0; i <= N_around; ++i) {
					float const a = 2 * Pi * i / float(N_around);
					m.position.push_back(c + r.x * std::cos(a) * u + r.y * std::sin(a) * v);
					m.normal.push_back(n); m.uv.push_back({0, 0});
				}
				for (int i = 0; i < N_around; ++i) {
					unsigned const a = kc + 1 + i, b = kc + 2 + i;
					if (end == 0) m.connectivity.push_back({kc, b, a});
					else m.connectivity.push_back({kc, a, b});
				}
			}
		}
		paint(m, color);
		return m;
	}

	mesh ellipsoid(vec3 const& center, vec3 const& radius, vec3 const& ex_arg, vec3 const& ey_arg, vec3 const& color, int Nu, int Nv)
	{
		vec3 const ex = normalize(ex_arg);
		vec3 const ey = normalize(ey_arg - dot(ey_arg, ex) * ex);
		vec3 const ez = cross(ex, ey);
		mesh m = mesh_primitive_sphere(1.0f, {0, 0, 0}, Nu, Nv);
		for (size_t k = 0; k < m.position.size(); ++k) {
			vec3 const p = m.position[k];
			vec3 const n = m.normal[k];
			m.position[k] = center + radius.x * p.x * ex + radius.y * p.y * ey + radius.z * p.z * ez;
			m.normal[k] = normalize(n.x / radius.x * ex + n.y / radius.y * ey + n.z / radius.z * ez);
		}
		paint(m, color);
		return m;
	}

	mesh cone(vec3 const& base, vec3 const& apex, vec2 const& radius, vec3 const& side, vec3 const& color, int N)
	{
		return tapered_tube(base, apex, radius, {0.002f, 0.002f}, side, color, N, 3, true);
	}
}
