#pragma once

#include "cgp/cgp.hpp"

// Wind field: constant mean direction + gusts in time + spatial turbulence.
//   w(p,t) = strength * gust(t) * dir + strength * turbulence * noise3(p, t)
struct wind_structure
{
	float strength = 7.5f;        // mean speed (m/s)
	float direction_deg = 205.0f; // horizontal direction the wind blows TOWARD (0 = +x, 90 = +y)
	float elevation_deg = 6.0f;   // small upward component (updraft along the facade)
	float turbulence = 0.8f;      // relative amplitude of the spatial/temporal noise
	float gust_amplitude = 0.6f;  // relative amplitude of the gusts
	float gust_frequency = 0.35f; // characteristic frequency of the gusts (Hz)

	float gust_factor = 1.0f;     // current gust multiplier (updated in update(), shown in the GUI)
	float time = 0.0f;

	void update(float t);                  // compute the gust factor for time t
	cgp::vec3 direction() const;           // unit mean direction
	cgp::vec3 mean() const;                // mean wind (with current gust), used e.g. for the rain
	cgp::vec3 at(cgp::vec3 const& p) const; // full wind at position p
};

// Smooth value noise in [-1,1] (cheap alternative to Perlin noise for per-vertex evaluation)
float value_noise(cgp::vec3 const& p);
float value_noise(float x);
