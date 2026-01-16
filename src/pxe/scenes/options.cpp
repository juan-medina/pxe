// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/audio_slider.hpp>
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

	if(const auto err = register_component<audio_slider>().unwrap(music_slider_); err) {
		return error("failed to register music slider component", *err);
	}

	if(const auto err = register_component<audio_slider>().unwrap(sfx_slider_); err) {
		return error("failed to register sfx slider component", *err);
	}

	std::shared_ptr<audio_slider> music_slider_component;
	if(const auto err = get_component<audio_slider>(music_slider_).unwrap(music_slider_component); err) {
		return error("failed to get music slider component", *err);
	}

	music_slider_component->set_label("Music:");
	music_slider_component->set_label_width(40);
	music_slider_component->set_slider_width(70);
	music_slider_component->set_muted(false);
	music_slider_component->set_value(50.0F);

	std::shared_ptr<audio_slider> sfx_slider_component;
	if(const auto err = get_component<audio_slider>(sfx_slider_).unwrap(sfx_slider_component); err) {
		return error("failed to get sfx slider component", *err);
	}

	sfx_slider_component->set_label("SFX:");
	sfx_slider_component->set_label_width(40);
	sfx_slider_component->set_slider_width(70);
	sfx_slider_component->set_muted(false);
	sfx_slider_component->set_value(50.0F);

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

	std::shared_ptr<audio_slider> music_slider_component;
	if(const auto err = get_component<audio_slider>(music_slider_).unwrap(music_slider_component); err) {
		return error("failed to get music slider component", *err);
	}

	// position the music slider inside the window
	const auto [slider_width, slider_height] = music_slider_component->get_size();

	float slider_y = (screen_height_ / 2) - (slider_height );

	// center slider in the window
	music_slider_component->set_position({.x = (screen_width_ / 2) - (slider_width / 2), .y = slider_y});

	std::shared_ptr<audio_slider> sfx_slider_component;
	if(const auto err = get_component<audio_slider>(sfx_slider_).unwrap(sfx_slider_component); err) {
		return error("failed to get sfx slider component", *err);
	}

	// position the sfx slider below the music slider
	slider_y += slider_height + 20.0F; // 20 pixels gap
	sfx_slider_component->set_position({.x = (screen_width_ / 2) - (slider_width / 2), .y = slider_y});

	return scene::layout(screen_size);
}

auto options::on_close_window() -> result<> {
	get_app().post_event(options_closed{});
	return true;
}

} // namespace pxe