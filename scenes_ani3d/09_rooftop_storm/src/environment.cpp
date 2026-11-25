#include "environment.hpp"

// Default window / loop settings
float project::gui_scale = 1.0f;
bool project::fps_limiting = true;
float project::fps_max = 60.0f;
bool project::vsync = true;
float project::initial_window_size_width = 0.75f;
float project::initial_window_size_height = 0.75f;

std::string project::path = "";


void environment_structure::send_opengl_uniform(opengl_shader_structure const& shader, bool expected) const
{
	// The scene uses several custom shaders that don't all need every uniform:
	//  everything here is sent as "optional" (expected = false).
	(void)expected;
	opengl_uniform(shader, "projection", camera_projection, false);
	opengl_uniform(shader, "view", camera_view, false);
	opengl_uniform(shader, "camera_position", camera_position, false);

	opengl_uniform(shader, "moon_direction", moon_direction, false);
	opengl_uniform(shader, "moon_color", moon_color, false);
	opengl_uniform(shader, "ambient_color", ambient_color, false);

	opengl_uniform(shader, "flash_direction", flash_direction, false);
	opengl_uniform(shader, "flash_color", flash_color, false);
	opengl_uniform(shader, "flash_intensity", flash_intensity, false);

	opengl_uniform(shader, "fog_color", fog_color, false);
	opengl_uniform(shader, "fog_density", fog_density, false);
	opengl_uniform(shader, "time", time, false);
	// Reset per-object values (a draw call can override them with its additional uniforms)
	opengl_uniform(shader, "emissive", vec3{0, 0, 0}, false);
}
