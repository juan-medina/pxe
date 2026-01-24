// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/sprite.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/banner.hpp>
#include <pxe/scenes/scene.hpp>

#include <memory>

namespace pxe {

auto banner::init(app &app) -> result<> {
	if(const auto err = scene::init(app).unwrap(); err) {
		return error("Failed to initialize base scene", err);
	}

	if(const auto err = register_component<sprite>(sprite_sheet_name, logo_frame).unwrap(logo_); err) {
		return error("Failed to register logo sprite", err);
	}

	return true;
}

auto banner::layout(const size screen_size) -> result<> {
	if(const auto err = scene::layout(screen_size).unwrap(); err) {
		return error("Failed to layout base scene", err);
	}

	std::shared_ptr<sprite> sprite_component;
	if(const auto err = get_component<sprite>(logo_).unwrap(sprite_component); err) {
		return error("Failed to get logo sprite component", err);
	}

	sprite_component->set_position({.x = screen_size.width / 2.0F, .y = screen_size.height / 2.0F});

	return true;
}

auto banner::update(const float delta) -> result<> {
	if(const auto err = scene::update(delta).unwrap(); err) {
		return error("Failed to update base scene: {}", err);
	}

	if(!is_enabled() || !is_visible()) {
		return true;
	}

	total_time_ += delta;

	if(total_time_ >= time_to_show) {
		get_app().post_event(finished{});
	}

	return true;
}

} // namespace pxe