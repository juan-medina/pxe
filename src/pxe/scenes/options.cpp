// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/component.hpp>
#include <pxe/components/window.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/options.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <memory>
#include <raygui.h>

namespace pxe {

auto options::init(app &app) -> result<> {
	if(const auto err = scene::init(app).unwrap(); err) {
		return error("failed to initialize options scene", *err);
	}

	if(const auto err = register_component<window>().unwrap(window_); err) {
		return error("failed to register window component", *err);
	}

	std::shared_ptr<window> window_component;
	if(const auto err = get_component<window>(window_).unwrap(window_component); err) {
		return error("failed to get window component", *err);
	}
	window_component->set_title("Options");
	window_component->set_size({.width = window_width, .height = window_height});

	close_window_ = app.on_event<window::close>(this, &options::on_close_window);

	return true;
}

auto options::end() -> result<> {
	get_app().unsubscribe(close_window_);
	return scene::end();
}

auto options::draw() -> result<> {
	if(!is_visible()) {
		return true;
	}

	DrawRectangle(0, 0, static_cast<int>(screen_width_), static_cast<int>(screen_height_), bg_color_);

	return scene::draw();
}

auto options::layout(const size screen_size) -> result<> {
	screen_width_ = screen_size.width;
	screen_height_ = screen_size.height;

	std::shared_ptr<window> window_component;
	if(const auto err = get_component<window>(window_).unwrap(window_component); err) {
		return error("failed to get window component", *err);
	}

	// center the window
	const auto [width, height] = window_component->get_size();
	window_component->set_position({.x = (screen_width_ - width) / 2, .y = (screen_height_ - height) / 2});

	return scene::layout(screen_size);
}

auto options::on_close_window() -> result<> {
	get_app().post_event(options_closed{});
	return true;
}

} // namespace pxe