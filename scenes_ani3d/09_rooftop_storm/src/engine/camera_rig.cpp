#include "camera_rig.hpp"

#include <algorithm>

using namespace cgp;

static float const pitch_limit = 1.50f;

vec3 camera_rig::forward() const
{
	return vec3{std::cos(pitch) * std::cos(yaw), std::cos(pitch) * std::sin(yaw), std::sin(pitch)};
}

vec3 camera_rig::position() const
{
	if (mode == mode_type::orbit)
		return target - distance * forward();
	return free_position;
}

void camera_rig::look_at(vec3 const& eye, vec3 const& center)
{
	vec3 const d = center - eye;
	distance = norm(d);
	vec3 const f = d / distance;
	yaw = std::atan2(f.y, f.x);
	pitch = std::asin(std::clamp(f.z, -1.0f, 1.0f));
	target = center;
	free_position = eye;
}

void camera_rig::set_mode(mode_type m)
{
	if (m == mode)
		return;
	if (m == mode_type::free) // keep the same viewpoint when switching
		free_position = position();
	else
		target = free_position + distance * forward();
	mode = m;
}

mat4 camera_rig::matrix_view() const
{
	vec3 const eye = position();
	vec3 const f = forward();
	vec3 const r = normalize(cross(f, vec3{0, 0, 1}));
	vec3 const u = cross(r, f);
	// Rows of the rotation = camera axes (OpenGL convention: camera looks toward -z)
	mat4 V = mat4::build_identity();
	V(0, 0) = r.x;  V(0, 1) = r.y;  V(0, 2) = r.z;  V(0, 3) = -dot(r, eye);
	V(1, 0) = u.x;  V(1, 1) = u.y;  V(1, 2) = u.z;  V(1, 3) = -dot(u, eye);
	V(2, 0) = -f.x; V(2, 1) = -f.y; V(2, 2) = -f.z; V(2, 3) = dot(f, eye);
	return V;
}

void camera_rig::mouse_move(input_devices const& inputs)
{
	if (inputs.mouse.on_gui)
		return;
	vec2 const dp = inputs.mouse.position.current - inputs.mouse.position.previous;
	bool const left = inputs.mouse.click.left;
	bool const right = inputs.mouse.click.right;
	bool const ctrl = inputs.keyboard.ctrl;

	if (left && !ctrl) {
		yaw -= 2.0f * dp.x;
		pitch = std::clamp(pitch - 1.5f * dp.y, -pitch_limit, pitch_limit);
	}
	else if (mode == mode_type::orbit && left && ctrl) {
		// Pan the target in the view plane
		vec3 const f = forward();
		vec3 const r = normalize(cross(f, vec3{0, 0, 1}));
		vec3 const u = cross(r, f);
		target -= 0.5f * distance * (dp.x * r + dp.y * u);
	}
	else if (mode == mode_type::orbit && right) {
		distance = std::clamp(distance * (1.0f + 1.5f * dp.y), 0.8f, 400.0f);
	}
}

void camera_rig::mouse_scroll(float scroll)
{
	if (mode == mode_type::orbit)
		distance = std::clamp(distance * (1.0f - 0.1f * scroll), 0.8f, 400.0f);
	else
		free_speed = std::clamp(free_speed * (1.0f + 0.15f * scroll), 0.5f, 200.0f);
}

void camera_rig::update(input_devices const& inputs, float dt)
{
	auto const& k = inputs.keyboard;
	if (k.ctrl) // Ctrl is used for GUI-free shortcuts, ignore motion
		return;

	vec3 const f = forward();
	vec3 const flat_f = normalize(vec3{f.x, f.y, 0.0f});
	vec3 const r = normalize(cross(f, vec3{0, 0, 1}));

	// GLFW key codes are physical positions (US layout): WASD = ZQSD on AZERTY
	vec3 move = {0, 0, 0};
	if (k.is_pressed(GLFW_KEY_W) || k.up)    move += flat_f;
	if (k.is_pressed(GLFW_KEY_S) || k.down)  move -= flat_f;
	if (k.is_pressed(GLFW_KEY_D) || k.right) move += r;
	if (k.is_pressed(GLFW_KEY_A) || k.left)  move -= r;
	if (k.is_pressed(GLFW_KEY_E) || k.is_pressed(GLFW_KEY_SPACE))         move.z += 1;
	if (k.is_pressed(GLFW_KEY_C))                                        move.z -= 1;

	if (norm(move) < 1e-4f)
		return;
	float const speed = (mode == mode_type::free ? free_speed : 0.6f * distance) * (k.shift ? 4.0f : 1.0f);
	vec3 const delta = speed * dt * move;
	if (mode == mode_type::free)
		free_position += delta;
	else
		target += delta;
}
