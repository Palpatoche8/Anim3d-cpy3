#include "scene.hpp"

#include <algorithm>
#include <cmath>

#include <chrono>
#include <cstdlib>

namespace {
	using clock_type = std::chrono::high_resolution_clock;
	float ms_since(clock_type::time_point t0) { return std::chrono::duration<float, std::milli>(clock_type::now() - t0).count(); }
	void smooth(float& v, float x) { v = 0.95f * v + 0.05f * x; }
}

using namespace cgp;

void scene_structure::initialize()
{
	std::string const sh = project::path + "shaders/";
	shader_lit.load(sh + "lit/lit.vert.glsl", sh + "lit/lit.frag.glsl");
	shader_building.load(sh + "lit/lit.vert.glsl", sh + "building/building.frag.glsl");
	shader_sky.load(sh + "sky/sky.vert.glsl", sh + "sky/sky.frag.glsl");
	shader_rain.load(sh + "rain/rain.vert.glsl", sh + "rain/rain.frag.glsl");
	shader_bolt.load(sh + "bolt/bolt.vert.glsl", sh + "bolt/bolt.frag.glsl");

	camera_projection.field_of_view = 48.0f * Pi / 180.0f;
	camera_projection.depth_min = 0.1f;
	camera_projection.depth_max = 3000.0f;
	reset_camera();

	sky.initialize_data_on_gpu(mesh_primitive_sphere(1.0f, {0, 0, 0}, 48, 24), shader_sky);

	city.initialize(shader_building);
	rooftop.initialize(shader_lit);
	character.initialize(rooftop.stand_point, shader_lit);

	collider_sphere.initialize_data_on_gpu(mesh_primitive_sphere(1.0f, {0, 0, 0}, 16, 8), shader_lit);
	collider_sphere.material.color = {0.8f, 0.15f, 0.1f};
	collider_sphere.material.alpha = 1.0f;

	cape.initialize(character, shader_lit);
	lightning.initialize(shader_bolt);
	rain.initialize(shader_rain, camera.position());

	if (std::getenv("ROOFTOP_NO_GUI") != nullptr) // used for automatic screenshots
		gui.show_gui = false;

	std::cout << "City: " << city.building_count << " buildings, cape: " << cape.x.size()
		<< " particles / " << cape.constraints.size() << " constraints" << std::endl;
}

void scene_structure::reset_camera()
{
	camera.mode = camera_rig::mode_type::orbit;
	// Low 3/4 back view: the silhouette stands out against the sky, the city below
	camera.look_at({-3.1f, -3.9f, 1.05f}, rooftop.stand_point + vec3{0.45f, 0.1f, 1.55f});
}

void scene_structure::reset_simulation()
{
	cape.initialize(character, shader_lit);
	wind.update(simulated_time);
	simulation_running = true;
}

void scene_structure::display_frame()
{
	float const dt = std::min(inputs.time_interval, 0.1f) * gui.time_scale;

	// Camera and environment
	camera.update(inputs, inputs.time_interval);
	environment.camera_view = camera.matrix_view();
	environment.camera_position = camera.position();

	// Simulation / effects update
	if (simulation_running) {
		simulated_time += dt;
		wind.update(simulated_time);
		auto t0 = clock_type::now();
		cape.step(dt, wind, character, rooftop);
		cape.update_drawable();
		smooth(perf.cape, ms_since(t0));
		lightning.update(dt, camera.position(), camera.forward());
		t0 = clock_type::now();
		if (gui.display_rain)
			rain.update(dt, wind.mean(), camera.position());
		smooth(perf.rain, ms_since(t0));
	}
	auto const t_render = clock_type::now();

	environment.time = simulated_time;
	environment.flash_intensity = lightning.flash;
	environment.flash_direction = normalize(lightning.strike_position - rooftop.stand_point);
	vec3 const bg = environment.fog_color + lightning.flash * 0.3f * environment.flash_color;
	environment.background_color = bg;

	// 1. Sky (no depth test, no depth write: always behind)
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	draw(sky, environment, 1, false);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	// 2. Opaque geometry
	draw(city.drawable, environment, 1, false);
	draw(rooftop.drawable, environment, 1, false);
	draw(character.body, environment, 1, false);
	uniform_generic_structure eyes_uniform;
	eyes_uniform.uniform_vec3["emissive"] = vec3{0.55f, 0.65f, 0.85f} * (0.6f + 0.4f * std::sin(3.0f * simulated_time) * 0.1f);
	draw(character.eyes, environment, 1, false, eyes_uniform);

	glDisable(GL_CULL_FACE); // the cape is double sided
	draw(cape.drawable, environment, 1, false);
	if (gui.display_wireframe_cape)
		draw_wireframe(cape.drawable, environment, {0.2f, 0.5f, 1.0f}, 1, false);

	if (gui.display_colliders) {
		for (capsule const& c : character.colliders) {
			for (int k = 0; k <= 3; ++k) {
				collider_sphere.model.translation = c.a + (k / 3.0f) * (c.b - c.a);
				collider_sphere.model.scaling = c.r;
				draw_wireframe(collider_sphere, environment, {1.0f, 0.3f, 0.2f}, 1, false);
			}
		}
	}

	// 3. Transparent / additive effects
	lightning.draw(environment, camera.position());
	if (gui.display_rain)
		rain.draw(environment);
	smooth(perf.render, ms_since(t_render));
	smooth(perf.frame, 1000.0f * inputs.time_interval);

	static int frame_count = 0;
	if (std::getenv("ROOFTOP_PROFILE") != nullptr && ++frame_count % 30 == 0)
		std::cout << "frame " << perf.frame << " ms | cape " << perf.cape << " | rain " << perf.rain << " | render (CPU submit) " << perf.render << std::endl;
}

void scene_structure::display_gui()
{
	ImGui::Text("L: lightning  P: pause  R: reset cape  TAB: camera mode  H: hide GUI");
	ImGui::Spacing();

	ImGui::Text("Frame %.1f ms | cape %.1f | rain %.1f | draw %.1f", perf.frame, perf.cape, perf.rain, perf.render);

	if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Running", &simulation_running);
		ImGui::SameLine();
		if (ImGui::Button("Reset cape"))
			reset_simulation();
		ImGui::SliderFloat("Time scale", &gui.time_scale, 0.05f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		if (cape.diverged)
			ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Cape diverged: press Reset");
	}

	if (ImGui::CollapsingHeader("Wind", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Strength (m/s)", &wind.strength, 0.0f, 25.0f);
		ImGui::SliderFloat("Direction (deg)", &wind.direction_deg, 0.0f, 360.0f);
		ImGui::SliderFloat("Elevation (deg)", &wind.elevation_deg, -30.0f, 45.0f);
		ImGui::SliderFloat("Turbulence", &wind.turbulence, 0.0f, 1.5f);
		ImGui::SliderFloat("Gust amplitude", &wind.gust_amplitude, 0.0f, 1.5f);
		ImGui::SliderFloat("Gust frequency", &wind.gust_frequency, 0.02f, 2.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		ImGui::ProgressBar(std::min(wind.gust_factor / 2.5f, 1.0f), ImVec2(-1, 0), ("gust x" + str(wind.gust_factor)).c_str());
	}

	if (ImGui::CollapsingHeader("Cape", ImGuiTreeNodeFlags_DefaultOpen)) {
		auto& P = cape.param;
		ImGui::SliderFloat("Structural", &P.stiffness_structural, 0.05f, 1.0f);
		ImGui::SliderFloat("Shear", &P.stiffness_shear, 0.0f, 1.0f);
		ImGui::SliderFloat("Bending", &P.stiffness_bend, 0.0f, 1.0f);
		ImGui::SliderFloat("Damping", &P.damping, 0.0f, 5.0f);
		ImGui::SliderInt("Iterations", &P.iterations, 1, 40);
		ImGui::SliderInt("Substeps", &P.substeps, 1, 10);
		ImGui::SliderFloat("Air drag", &P.drag_normal, 0.0f, 3.0f);
		ImGui::SliderFloat("Mass (kg)", &P.mass, 0.3f, 5.0f);
		bool rebuild = false;
		rebuild |= ImGui::SliderInt("Resolution U", &P.nu, 6, 48);
		rebuild |= ImGui::SliderInt("Resolution V", &P.nv, 6, 64);
		rebuild |= ImGui::SliderFloat("Length (m)", &P.length, 0.8f, 2.2f);
		rebuild |= ImGui::SliderFloat("Hem width (m)", &P.width_bottom, 0.6f, 2.6f);
		if (rebuild)
			reset_simulation();
		// Note: the mass per particle depends on the resolution, it is updated at reset
		ImGui::Text("%d particles, %d constraints", int(cape.x.size()), int(cape.constraints.size()));
	}

	if (ImGui::CollapsingHeader("Lightning & rain", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (ImGui::Button("Lightning !"))
			lightning.trigger(camera.position(), camera.forward());
		ImGui::SameLine();
		ImGui::Checkbox("Automatic", &lightning.automatic);
		ImGui::SliderFloat("Mean interval (s)", &lightning.mean_interval, 1.5f, 30.0f);
		ImGui::SliderFloat("Flash strength", &lightning.flash_strength, 0.2f, 5.0f);
		ImGui::SliderFloat("Branching", &lightning.branch_probability, 0.0f, 0.8f);
		ImGui::Checkbox("Rain", &gui.display_rain);
		ImGui::SliderFloat("Rain density", &rain.density, 0.0f, 1.0f);
		ImGui::SliderFloat("Rain wind influence", &rain.wind_influence, 0.0f, 1.5f);
	}

	if (ImGui::CollapsingHeader("Camera & atmosphere")) {
		int mode = camera.mode == camera_rig::mode_type::orbit ? 0 : 1;
		if (ImGui::RadioButton("Orbit", mode == 0)) camera.set_mode(camera_rig::mode_type::orbit);
		ImGui::SameLine();
		if (ImGui::RadioButton("Free (WASD/ZQSD)", mode == 1)) camera.set_mode(camera_rig::mode_type::free);
		if (ImGui::Button("Reset camera"))
			reset_camera();
		ImGui::SliderFloat("Fog density", &environment.fog_density, 0.0f, 0.03f, "%.4f");
		ImGui::ColorEdit3("Moon color", &environment.moon_color.x);
		ImGui::ColorEdit3("Fog color", &environment.fog_color.x);
		ImGui::Checkbox("Cape wireframe", &gui.display_wireframe_cape);
		ImGui::Checkbox("Show body colliders", &gui.display_colliders);
	}
}

void scene_structure::mouse_move_event()
{
	camera.mouse_move(inputs);
}

void scene_structure::mouse_click_event()
{
}

void scene_structure::mouse_scroll_event()
{
	camera.mouse_scroll(inputs.mouse.scroll);
}

void scene_structure::keyboard_event()
{
	auto const& last = inputs.keyboard.last_action;
	if (last.is_pressed(GLFW_KEY_L))
		lightning.trigger(camera.position(), camera.forward());
	if (last.is_pressed(GLFW_KEY_P))
		simulation_running = !simulation_running;
	if (last.is_pressed(GLFW_KEY_R) && !inputs.keyboard.shift)
		reset_simulation();
	if (last.is_pressed(GLFW_KEY_H))
		gui.show_gui = !gui.show_gui;
	if (last.is_pressed(GLFW_KEY_TAB))
		camera.set_mode(camera.mode == camera_rig::mode_type::orbit ? camera_rig::mode_type::free : camera_rig::mode_type::orbit);
}

void scene_structure::idle_frame()
{
}
