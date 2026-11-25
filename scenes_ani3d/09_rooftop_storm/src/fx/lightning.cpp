#include "lightning.hpp"

using namespace cgp;

float lightning_structure::rand(float a, float b)
{
	return std::uniform_real_distribution<float>(a, b)(rng);
}

void lightning_structure::initialize(opengl_shader_structure const& shader_arg)
{
	shader = shader_arg;
	// vertex = position (3) + (intensity, across coordinate in [-1,1]) (2)
	buffer.initialize({3, 2});
}

// Recursive midpoint displacement. Each level displaces the middle of the segment
// perpendicularly, by an amount proportional to the segment length; branches are spawned randomly.
void lightning_structure::subdivide(vec3 const& a, vec3 const& b, float offset, int depth, float intensity, float width)
{
	if (depth == 0) {
		segments.push_back({a, b, intensity, width});
		return;
	}
	vec3 const ab = b - a;
	vec3 const dir = normalize(ab);
	vec3 r = {rand(-1, 1), rand(-1, 1), rand(-1, 1)};
	r -= dot(r, dir) * dir;
	vec3 const mid = 0.5f * (a + b) + offset * r;

	subdivide(a, mid, 0.5f * offset, depth - 1, intensity, width);
	subdivide(mid, b, 0.5f * offset, depth - 1, intensity, width);

	// Branch: continues roughly in the direction of the first half, deviated, shorter and dimmer
	if (depth >= 3 && depth <= subdivisions - 1 && rand() < branch_probability && intensity > 0.15f) {
		vec3 const d1 = mid - a;
		vec3 dev = {rand(-1, 1), rand(-1, 1), rand(-0.3f, 0.3f)};
		vec3 const end = mid + 0.9f * d1 + 0.6f * norm(d1) * dev + vec3{0, 0, -0.3f * norm(d1)};
		subdivide(mid, end, 0.5f * offset, depth - 1, intensity * 0.45f, width * 0.6f);
	}
}

void lightning_structure::generate_bolt(vec3 const& start, vec3 const& end)
{
	segments.clear();
	float const L = norm(end - start);
	subdivide(start, end, 0.22f * L, subdivisions, 1.0f, 2.2f);
}

void lightning_structure::trigger(vec3 const& camera_position, vec3 const& camera_forward)
{
	// Staging: most strikes happen far away in the field of view of the camera
	// (behind the character in the default shot), the others anywhere around.
	float const view_az = std::atan2(camera_forward.y, camera_forward.x);
	float const az = rand() < 0.8f ? view_az + rand(-0.45f, 0.45f) : rand(-Pi, Pi);
	float const dist = rand(200.0f, 450.0f);
	vec3 const ground = vec3{camera_position.x, camera_position.y, 0.0f} + vec3{dist * std::cos(az), dist * std::sin(az), rand(-60.0f, 20.0f)};
	vec3 const top = ground + vec3{rand(-80.0f, 80.0f), rand(-80.0f, 80.0f), rand(260.0f, 330.0f)};

	cloud_only = strike_count > 0 && rand() < 0.25f; // the first one is always a visible bolt
	strike_count++;
	generate_bolt(top, ground);
	strike_position = cloud_only ? top : 0.5f * (top + ground);

	// Flash envelope: main stroke + 1 to 3 return strokes
	strokes.clear();
	float const peak = (cloud_only ? 0.45f : 1.0f) * flash_strength;
	strokes.push_back({0.0f, peak});
	int const n_restrokes = 1 + int(rand(0.0f, 2.99f));
	float t = 0.0f;
	for (int k = 0; k < n_restrokes; ++k) {
		t += rand(0.05f, 0.16f);
		strokes.push_back({t, peak * rand(0.35f, 0.85f)});
	}
	time_since_strike = 0.0f;
}

void lightning_structure::update(float dt, vec3 const& camera_position, vec3 const& camera_forward)
{
	if (automatic) {
		next_strike_in -= dt;
		if (next_strike_in <= 0.0f) {
			trigger(camera_position, camera_forward);
			// Exponential law (Poisson process), with a minimal delay
			float const u = std::max(1e-4f, rand());
			next_strike_in = std::max(1.2f, -std::log(u) * mean_interval);
		}
	}

	time_since_strike += dt;
	float const t = time_since_strike;
	float f = 0.0f;
	for (stroke const& s : strokes) {
		if (t < s.t0) continue;
		float const dt_s = t - s.t0;
		// Very fast rise, fast exponential decay + small high frequency flicker
		float const rise = std::min(1.0f, dt_s / 0.012f);
		f += s.amplitude * rise * std::exp(-dt_s / 0.07f);
	}
	// Faint afterglow tail
	if (!strokes.empty())
		f += 0.12f * strokes[0].amplitude * std::exp(-t / 0.35f);
	float const flicker = 0.85f + 0.15f * std::sin(t * 157.0f) * std::sin(t * 61.0f);
	flash = f * flicker;
	bolt_visibility = cloud_only ? 0.0f : std::clamp(f / std::max(0.01f, flash_strength) * 1.6f, 0.0f, 1.5f);
	if (t > 1.2f)
		bolt_visibility = 0.0f;
}

void lightning_structure::draw(environment_generic_structure const& environment, vec3 const& camera_position)
{
	if (bolt_visibility <= 0.01f || segments.empty())
		return;

	// Camera facing ribbons (two triangles per segment)
	std::vector<float> data;
	data.reserve(segments.size() * 6 * 5);
	auto push = [&](vec3 const& p, float intensity, float across) {
		data.push_back(p.x); data.push_back(p.y); data.push_back(p.z);
		data.push_back(intensity); data.push_back(across);
	};
	for (segment const& s : segments) {
		vec3 const d = s.b - s.a;
		vec3 side = cross(d, camera_position - s.a);
		float const ns = norm(side);
		if (ns < 1e-6f) continue;
		side = side / ns;
		// Glow halo width grows with the distance to stay visible
		float const halo = s.width * (1.0f + 0.004f * norm(camera_position - s.a)) * 4.0f;
		vec3 const o = halo * side;
		float const I = s.intensity * bolt_visibility;
		push(s.a - o, I, -1); push(s.a + o, I, 1); push(s.b + o, I, 1);
		push(s.a - o, I, -1); push(s.b + o, I, 1); push(s.b - o, I, -1);
	}
	buffer.update(data);

	glUseProgram(shader.id);
	environment.send_opengl_uniform(shader, false);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive glow
	glDepthMask(GL_FALSE);
	buffer.draw(GL_TRIANGLES);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glUseProgram(0);
}
