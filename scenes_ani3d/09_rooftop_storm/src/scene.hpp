#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

#include "engine/camera_rig.hpp"
#include "world/city.hpp"
#include "world/rooftop.hpp"
#include "world/character.hpp"
#include "sim/wind.hpp"
#include "sim/cape.hpp"
#include "fx/lightning.hpp"
#include "fx/rain.hpp"

struct gui_parameters {
	bool show_gui = true;
	bool display_wireframe_cape = false;
	bool display_colliders = false;
	bool display_rain = true;
	float time_scale = 1.0f;
};

// Rooftop storm scene: a caped vigilante on the edge of a roof, above the city, in a thunderstorm.
struct scene_structure : cgp::scene_inputs_generic {

	// Base elements required by the application loop
	camera_projection_perspective camera_projection;
	window_structure window;
	environment_structure environment;
	input_devices inputs;
	gui_parameters gui;
	camera_rig camera;

	// Shaders
	opengl_shader_structure shader_lit, shader_building, shader_sky, shader_rain, shader_bolt;

	// World
	mesh_drawable sky;
	city_structure city;
	rooftop_structure rooftop;
	character_structure character;
	mesh_drawable collider_sphere; // debug display of the collision capsules

	// Simulation and effects
	wind_structure wind;
	cape_structure cape;
	lightning_structure lightning;
	rain_structure rain;

	bool simulation_running = true;
	float simulated_time = 0.0f;

	void initialize();
	void display_frame();
	void display_gui();
	void reset_simulation();
	void reset_camera();

	void mouse_move_event();
	void mouse_click_event();
	void mouse_scroll_event();
	void keyboard_event();
	void idle_frame();
};
