#include "character.hpp"
#include "engine/mesh_builder.hpp"

using namespace cgp;
using namespace mesh_builder;

// All dimensions in meters, local frame: x forward, y left, z up, feet at the origin.
void character_structure::initialize(vec3 const& feet_position, opengl_shader_structure const& shader)
{
	origin = feet_position;
	vec3 const suit = {0.055f, 0.060f, 0.075f};
	vec3 const suit_dark = {0.035f, 0.038f, 0.048f};
	vec3 const belt = {0.13f, 0.12f, 0.09f};
	vec3 const Y = {0, 1, 0}, X = {1, 0, 0};

	mesh m;
	for (int side = -1; side <= 1; side += 2) {
		float const s = float(side);
		// Wide heroic stance: feet apart, one foot slightly forward
		vec3 const ankle = {0.06f * (s > 0 ? 1.0f : -0.3f), s * 0.22f, 0.09f};
		vec3 const knee = {0.05f, s * 0.165f, 0.52f};
		vec3 const hip = {0.0f, s * 0.11f, 0.96f};

		m.push_back(ellipsoid(ankle + vec3{0.07f, 0, -0.045f}, {0.14f, 0.06f, 0.05f}, X, Y, suit_dark, 14, 8)); // boot foot
		m.push_back(tapered_tube(ankle + vec3{0, 0, -0.05f}, knee, {0.065f, 0.07f}, {0.072f, 0.075f}, Y, suit_dark, 12, 3)); // boot + shin
		m.push_back(tapered_tube(knee, hip, {0.075f, 0.08f}, {0.10f, 0.11f}, Y, suit, 12, 3)); // thigh
		m.push_back(ellipsoid(knee, {0.08f, 0.075f, 0.07f}, X, Y, suit, 10, 8)); // knee joint

		// Arms slightly away from the body, fists clenched
		vec3 const shoulder = {0.0f, s * 0.33f, 1.45f};
		vec3 const elbow = {0.02f, s * 0.40f, 1.14f};
		vec3 const wrist = {0.07f, s * 0.38f, 0.88f};
		m.push_back(ellipsoid({0.0f, s * 0.29f, 1.48f}, {0.13f, 0.13f, 0.11f}, X, Y, suit, 16, 10)); // shoulder muscle
		m.push_back(tapered_tube(shoulder, elbow, {0.085f, 0.075f}, {0.06f, 0.06f}, X, suit, 12, 3));
		m.push_back(ellipsoid(elbow, {0.062f, 0.062f, 0.062f}, X, Y, suit, 10, 8));
		m.push_back(tapered_tube(elbow, wrist, {0.06f, 0.058f}, {0.052f, 0.05f}, X, suit_dark, 12, 3)); // gauntlet
		m.push_back(ellipsoid(wrist + vec3{0.01f, 0, -0.06f}, {0.055f, 0.045f, 0.065f}, X, Y, suit_dark, 12, 8)); // fist
	}

	// Pelvis, belt, torso (V-shaped: narrow waist, wide chest)
	m.push_back(ellipsoid({0, 0, 0.96f}, {0.14f, 0.19f, 0.12f}, X, Y, suit, 18, 10));
	m.push_back(tapered_tube({0, 0, 0.98f}, {0, 0, 1.06f}, {0.195f, 0.145f}, {0.19f, 0.14f}, Y, belt, 20, 1));
	m.push_back(tapered_tube({0, 0, 1.0f}, {0, 0, 1.47f}, {0.175f, 0.13f}, {0.30f, 0.17f}, Y, suit, 20, 4));
	m.push_back(ellipsoid({0, 0, 1.47f}, {0.17f, 0.30f, 0.09f}, X, Y, suit, 20, 10)); // top of the chest / trapezius
	m.push_back(ellipsoid({0.06f, 0, 1.32f}, {0.12f, 0.24f, 0.13f}, X, Y, suit, 18, 10)); // pectorals

	// Neck, head and hood
	m.push_back(tapered_tube({0, 0, 1.48f}, {0.0f, 0, 1.64f}, {0.075f, 0.07f}, {0.065f, 0.06f}, Y, suit, 12, 2));
	m.push_back(ellipsoid({0.015f, 0, 1.73f}, {0.11f, 0.092f, 0.125f}, X, Y, suit_dark, 18, 12)); // masked head
	m.push_back(ellipsoid({-0.02f, 0, 1.75f}, {0.13f, 0.125f, 0.15f}, X, Y, suit, 20, 12));       // hood shell
	m.push_back(cone({-0.09f, 0, 1.80f}, {-0.30f, 0, 1.64f}, {0.09f, 0.07f}, Y, suit, 12));        // drooping hood point
	m.push_back(tapered_tube({-0.03f, 0, 1.50f}, {-0.025f, 0, 1.66f}, {0.20f, 0.17f}, {0.13f, 0.12f}, Y, suit, 18, 2)); // raised collar / cowl drape
	m.push_back(cone({0.11f, 0, 1.70f}, {0.15f, 0, 1.66f}, {0.03f, 0.03f}, Y, suit_dark, 8)); // jaw guard (sharp profile)

	m.fill_empty_field();
	body.initialize_data_on_gpu(m, shader);
	body.model.translation = origin;
	body.material.phong.ambient = 1.0f;
	body.material.phong.diffuse = 1.0f;
	body.material.phong.specular = 0.35f;
	body.material.phong.specular_exponent = 40.0f;

	// Eyes: two thin slits in the hood
	mesh e;
	for (int side = -1; side <= 1; side += 2)
		e.push_back(ellipsoid({0.118f, side * 0.036f, 1.745f}, {0.012f, 0.024f, 0.007f}, X, vec3{0, 1, -0.25f * side}, {1, 1, 1}, 8, 6));
	e.fill_empty_field();
	eyes.initialize_data_on_gpu(e, shader);
	eyes.model.translation = origin;
	eyes.material.color = {0.75f, 0.85f, 1.0f};

	// Collision proxies (world space)
	colliders.clear();
	auto add = [&](vec3 const& a, vec3 const& b, float r) { colliders.push_back({origin + a, origin + b, r}); };
	for (int side = -1; side <= 1; side += 2) {
		float const s = float(side);
		add({0, s * 0.12f, 1.05f}, {0, s * 0.15f, 1.40f}, 0.16f);   // torso half
		add({0, s * 0.29f, 1.48f}, {0, s * 0.29f, 1.48f}, 0.13f);   // shoulder
		add({0, s * 0.33f, 1.45f}, {0.02f, s * 0.40f, 1.14f}, 0.085f); // upper arm
		add({0.02f, s * 0.40f, 1.14f}, {0.07f, s * 0.38f, 0.85f}, 0.07f); // forearm
		add({0, s * 0.11f, 0.96f}, {0.05f, s * 0.165f, 0.52f}, 0.11f);   // thigh
		add({0.05f, s * 0.165f, 0.52f}, {0.06f * (s > 0 ? 1.0f : -0.3f), s * 0.22f, 0.06f}, 0.08f); // shin
	}
	add({0, 0, 0.96f}, {0, 0, 0.96f}, 0.17f);       // pelvis
	add({-0.01f, 0, 1.50f}, {0, 0, 1.75f}, 0.14f);  // neck + head + hood
}

vec3 character_structure::cape_anchor(float s) const
{
	// Arc behind the neck, joining the back of both shoulders (slightly higher at the collar)
	float const c = 1.0f - s * s;
	return origin + vec3{-0.13f - 0.07f * c, 0.31f * s, 1.50f + 0.06f * c};
}
