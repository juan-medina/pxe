// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/quick_bar.hpp>
#include <pxe/components/version_display.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/game_overlay.hpp>
#include <pxe/scenes/scene.hpp>

#include <format>
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
	if(const auto err = register_component<quick_bar>(sprite_sheet, normal, hover, gap).unwrap(quick_bar_); err) {
		return error("failed to initialize quick bar", *err);
	}

	std::shared_ptr<quick_bar> quick_bar_ptr;
	if(const auto err = get_component<quick_bar>(quick_bar_).unwrap(quick_bar_ptr); err) {
		return error("failed to get quick bar", *err);
	}

	// Add options button
	if(const auto err = quick_bar_ptr->add_button("gear.png").unwrap(options_button_); err) {
		return error("failed to add options button", *err);
	}

	// Add fullscreen toggle button
	if(const auto err = quick_bar_ptr->add_button(fullscreen_frame).unwrap(toggle_fullscreen_button_); err) {
		return error("failed to add fullscreen toggle button", *err);
	}

#ifndef __EMSCRIPTEN__
	// Add close button
	if(const auto err = quick_bar_ptr->add_button("cross.png").unwrap(close_button_); err) {
		return error("failed to add close button", *err);
	}
#endif

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
	std::shared_ptr<quick_bar> quick_bar_ptr;
	if(const auto err = get_component<quick_bar>(quick_bar_).unwrap(quick_bar_ptr); err) {
		return error("failed to get quick bar for layout", *err);
	}

	auto [bar_width, bar_height] = quick_bar_ptr->get_size();

	const auto x = screen_size.width - (bar_width / 2) - bar_gap;
	const auto y = bar_gap + (bar_height / 2);
	quick_bar_ptr->set_position({
		.x = x,
		.y = y,
	});

	return scene::layout(screen_size);
}

auto game_overlay::on_button_click(const button::click &evt) -> result<> {
	if(evt.id == close_button_) {
		get_app().close();
	} else if(evt.id == toggle_fullscreen_button_) {
		auto const full_screen = get_app().toggle_fullscreen();
		std::shared_ptr<quick_bar> quick_bar_ptr;
		if(const auto err = get_component<quick_bar>(quick_bar_).unwrap(quick_bar_ptr); err) {
			return error("failed to get quick bar for layout", *err);
		}
		if(const auto err =
			   quick_bar_ptr
				   ->set_button_frame_name(toggle_fullscreen_button_, full_screen ? windowed_frame : fullscreen_frame)
				   .unwrap();
		   err) {
			return error("failed to set toggle fullscreen button frame", *err);
		}
	} else if(evt.id == version_display_) {
		get_app().post_event(version_click{});
	} else if(evt.id == options_button_) {
		get_app().post_event(options_click{});
	}
	return true;
}

} // namespace pxe
