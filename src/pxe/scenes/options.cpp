// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/audio_slider.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/window.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/options.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <cstddef>
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
	music_slider_component->set_label_width(audio_label_width);
	music_slider_component->set_slider_width(audio_slider_width);

	std::shared_ptr<audio_slider> sfx_slider_component;
	if(const auto err = get_component<audio_slider>(sfx_slider_).unwrap(sfx_slider_component); err) {
		return error("failed to get sfx slider component", *err);
	}

	sfx_slider_component->set_label("SFX:");
	sfx_slider_component->set_label_width(audio_label_width);
	sfx_slider_component->set_slider_width(audio_slider_width);

	slider_change_ = app.bind_event<audio_slider::audio_slider_changed>(this, &options::on_slider_change);

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

	float slider_y = (screen_height_ / 2) - (slider_height);

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

auto options::show() -> result<> {
	const auto music_volume = get_app().get_music_volume();
	const auto music_muted = get_app().is_music_muted();
	if(const auto err = set_slider_values(music_slider_, music_volume, music_muted).unwrap(); err) {
		return error("failed to set music slider values", *err);
	}

	const auto sfx_volume = get_app().get_sfx_volume();
	const auto sfx_muted = get_app().is_sfx_muted();
	if(const auto err = set_slider_values(sfx_slider_, sfx_volume, sfx_muted).unwrap(); err) {
		return error("failed to set sfx slider values", *err);
	}

	return scene::show();
}

auto options::on_close_window() -> result<> {
	get_app().post_event(options_closed{});
	return true;
}

auto options::on_slider_change(const audio_slider::audio_slider_changed &change) -> result<> {
	const auto value = static_cast<float>(change.value) / 100.0F;
	if(change.id == music_slider_) {
		get_app().set_music_volume(value);
		get_app().set_music_muted(change.muted);
	} else if(change.id == sfx_slider_) {
		get_app().set_sfx_volume(value);
		get_app().set_sfx_muted(change.muted);
	}

	return true;
}

auto options::set_slider_values(const size_t slider, const float value, const bool muted) -> result<> {
	std::shared_ptr<audio_slider> music_slider_component;
	if(const auto err = get_component<audio_slider>(slider).unwrap(music_slider_component); err) {
		return error("failed to get music slider component", *err);
	}

	music_slider_component->set_value(static_cast<int>(value * 100.0F));
	music_slider_component->set_muted(muted);

	return true;
}

} // namespace pxe