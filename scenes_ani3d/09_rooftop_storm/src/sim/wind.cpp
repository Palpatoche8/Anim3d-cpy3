#include "wind.hpp"

using namespace cgp;

static float hash(int x, int y, int z)
{
	unsigned int h = unsigned(x) * 374761393u + unsigned(y) * 668265263u + unsigned(z) * 2147483647u;
	h = (h ^ (h >> 13)) * 1274126177u;
	h ^= h >> 16;
	return (h & 0xFFFFFF) / float(0xFFFFFF) * 2.0f - 1.0f;
}

static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

float value_noise(vec3 const& p)
{
	int const ix = int(std::floor(p.x)), iy = int(std::floor(p.y)), iz = int(std::floor(p.z));
	float const fx = smooth(p.x - ix), fy = smooth(p.y - iy), fz = smooth(p.z - iz);
	auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
	float const x00 = lerp(hash(ix, iy, iz), hash(ix + 1, iy, iz), fx);
	float const x10 = lerp(hash(ix, iy + 1, iz), hash(ix + 1, iy + 1, iz), fx);
	float const x01 = lerp(hash(ix, iy, iz + 1), hash(ix + 1, iy, iz + 1), fx);
	float const x11 = lerp(hash(ix, iy + 1, iz + 1), hash(ix + 1, iy + 1, iz + 1), fx);
	return lerp(lerp(x00, x10, fy), lerp(x01, x11, fy), fz);
}

float value_noise(float x)
{
	return value_noise(vec3{x, 0.37f, 0.71f});
}

void wind_structure::update(float t)
{
	time = t;
	// Gusts: sum of two octaves of noise, sharpened so that calm phases alternate with strong bursts
	float const f = gust_frequency;
	float g = 0.65f * value_noise(t * f) + 0.35f * value_noise(t * f * 2.7f + 13.1f);
	g = g > 0 ? std::pow(g, 0.7f) : -0.5f * std::pow(-g, 1.3f);
	gust_factor = std::max(0.15f, 1.0f + gust_amplitude * 1.4f * g);
}

vec3 wind_structure::direction() const
{
	float const a = direction_deg * Pi / 180.0f, e = elevation_deg * Pi / 180.0f;
	return vec3{std::cos(e) * std::cos(a), std::cos(e) * std::sin(a), std::sin(e)};
}

vec3 wind_structure::mean() const
{
	return strength * gust_factor * direction();
}

vec3 wind_structure::at(vec3 const& p) const
{
	vec3 const d = direction();
	// Turbulence: noise advected with the wind (eddies travel with the flow), fast evolving in time
	vec3 const q = 1.8f * (p - 0.6f * time * strength * d) ;
	float const tt = 1.7f * time;
	vec3 const n = {
		value_noise(q + vec3{0, 0, tt}),
		value_noise(q + vec3{31.4f, 0, tt}),
		value_noise(q + vec3{0, 57.2f, tt}) };
	return strength * (gust_factor * d + turbulence * n);
}
