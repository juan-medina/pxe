// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/sprite_button.hpp>
#include <pxe/components/version_display.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/game_overlay.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <memory>
#include <optional>

namespace pxe {

auto game_overlay::init(app &app) -> result<> {
	if(const auto err = scene::init(app).unwrap(); err) {
		return error("failed to initialize base component", *err);
	}

	// Initialize version display
	if(const auto err = register_component<version_display>().unwrap(version_display_); err) {
		return error("failed to register version display component", *err);
	}

	// Initialize quick bar
	if(const auto err =
		   register_component<sprite_button>(sprite_sheet, sprite_frame, normal, hover).unwrap(options_button_);
	   err) {
		return error("failed to initialize quick bar", *err);
	}

	std::shared_ptr<sprite_button> options_button_ptr;
	if(const auto err = get_component<sprite_button>(options_button_).unwrap(options_button_ptr); err) {
		return error("failed to get quick bar", *err);
	}

	options_button_ptr->set_controller_button(GAMEPAD_BUTTON_MIDDLE_RIGHT);
	options_button_ptr->set_controller_button_alignment(vertical_alignment::bottom, horizontal_alignment::center);

	// all buttons clicks
	button_click_ = app.bind_event<button::click>(this, &game_overlay::on_button_click);

	return true;
}

auto game_overlay::end() -> result<> {
	get_app().unsubscribe(button_click_);
	return scene::end();
}

auto game_overlay::layout(const size screen_size) -> result<> {
	// Layout version display
	std::shared_ptr<version_display> version;
	if(const auto err = get_component<version_display>(version_display_).unwrap(version); err) {
		return error("failed to get version display component", err);
	}

	const auto [width, height] = version->get_size();
	version->set_position({
		.x = screen_size.width - width - margin,
		.y = screen_size.height - height - margin,
	});

	// Layout quick bar
	std::shared_ptr<sprite_button> options_button_ptr;
	if(const auto err = get_component<sprite_button>(options_button_).unwrap(options_button_ptr); err) {
		return error("failed to get quick bar for layout", *err);
	}

	auto [button_width, button_height] = options_button_ptr->get_size();

	const auto x = screen_size.width - (button_width / 2) - bar_gap;
	const auto y = bar_gap + (button_height / 2);
	options_button_ptr->set_position({
		.x = x,
		.y = y,
	});

	return scene::layout(screen_size);
}

auto game_overlay::on_button_click(const button::click &evt) -> result<> {
	if(evt.id == version_display_) {
		get_app().post_event(version_click{});
	} else if(evt.id == options_button_) {
		get_app().post_event(options_click{});
	}
	return true;
}

} // namespace pxe
