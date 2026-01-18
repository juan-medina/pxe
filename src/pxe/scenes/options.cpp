// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/audio_slider.hpp>
#include <pxe/components/checkbox.hpp>
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

	if(const auto err = register_component<checkbox>().unwrap(crt_cb_); err) {
		return error("failed to register crt checkbox component", *err);
	}
	std::shared_ptr<checkbox> checkbox_component;
	if(const auto err = get_component<checkbox>(crt_cb_).unwrap(checkbox_component); err) {
		return error("failed to get crt checkbox component", *err);
	}
	checkbox_component->set_title("Show CRT");

	if(const auto err = register_component<checkbox>().unwrap(scan_lines_cb_); err) {
		return error("failed to register scan lines checkbox component", *err);
	}
	if(const auto err = get_component<checkbox>(scan_lines_cb_).unwrap(checkbox_component); err) {
		return error("failed to get scan lines checkbox component", *err);
	}
	checkbox_component->set_title("Enable Scan Lines");

	if(const auto err = register_component<checkbox>().unwrap(color_bleed_cb_); err) {
		return error("failed to register color bleed checkbox component", *err);
	}
	if(const auto err = get_component<checkbox>(color_bleed_cb_).unwrap(checkbox_component); err) {
		return error("failed to get color bleed checkbox component", *err);
	}
	checkbox_component->set_title("Enable Color Bleed");

	checkbox_changed_ = app.bind_event<checkbox::checkbox_changed>(this, &options::on_checkbox_changed);

	return true;
}

auto options::end() -> result<> {
	get_app().unsubscribe(close_window_);
	get_app().unsubscribe(slider_change_);
	get_app().unsubscribe(checkbox_changed_);

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
	const auto window_y = (screen_height_ - height) / 2;
	window_component->set_position({.x = (screen_width_ - width) / 2, .y = window_y});

	std::shared_ptr<audio_slider> music_slider_component;
	if(const auto err = get_component<audio_slider>(music_slider_).unwrap(music_slider_component); err) {
		return error("failed to get music slider component", *err);
	}

	// position the music slider inside the window
	const auto [slider_width, slider_height] = music_slider_component->get_size();

	const float control_x = (screen_width_ / 2) - (slider_width / 2);
	float control_y = window_y + (slider_height * 4);

	// center slider in the window
	music_slider_component->set_position({.x = control_x, .y = control_y});

	std::shared_ptr<audio_slider> sfx_slider_component;
	if(const auto err = get_component<audio_slider>(sfx_slider_).unwrap(sfx_slider_component); err) {
		return error("failed to get sfx slider component", *err);
	}

	// position the sfx slider below the music slider
	control_y += slider_height + control_row_gap;
	sfx_slider_component->set_position({.x = control_x, .y = control_y});

	std::shared_ptr<checkbox> checkbox_component;
	if(const auto err = get_component<checkbox>(crt_cb_).unwrap(checkbox_component); err) {
		return error("failed to get crt checkbox component", *err);
	}

	// separate effects controls from audio controls
	control_y += control_row_gap;

	control_y += slider_height + control_row_gap;
	checkbox_component->set_position({.x = control_x, .y = control_y});

	if(const auto err = get_component<checkbox>(scan_lines_cb_).unwrap(checkbox_component); err) {
		return error("failed to get scan lines checkbox component", *err);
	}

	control_y += slider_height + control_row_gap;
	checkbox_component->set_position({.x = control_x, .y = control_y});

	if(const auto err = get_component<checkbox>(color_bleed_cb_).unwrap(checkbox_component); err) {
		return error("failed to get color bleed checkbox component", *err);
	}

	control_y += slider_height + control_row_gap;
	checkbox_component->set_position({.x = control_x, .y = control_y});

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

	const auto crt_enabled = get_app().is_crt_enabled();
	if(const auto err = set_checkbox_value(crt_cb_, crt_enabled).unwrap(); err) {
		return error("failed to set crt checkbox value", *err);
	}

	const auto scan_lines_enabled = get_app().is_scan_lines_enabled();
	if(const auto err = set_checkbox_value(scan_lines_cb_, scan_lines_enabled).unwrap(); err) {
		return error("failed to set scan lines checkbox value", *err);
	}

	const auto color_bleed_enabled = get_app().is_color_bleed_enabled();
	if(const auto err = set_checkbox_value(color_bleed_cb_, color_bleed_enabled).unwrap(); err) {
		return error("failed to set color bleed checkbox value", *err);
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

auto options::set_checkbox_value(const size_t cb, const bool value) -> result<> {
	std::shared_ptr<checkbox> checkbox_component;
	if(const auto err = get_component<checkbox>(cb).unwrap(checkbox_component); err) {
		return error("failed to get checkbox component", *err);
	}

	checkbox_component->set_checked(value);
	return true;
}

auto options::on_checkbox_changed(const checkbox::checkbox_changed &change) -> result<> {
	if(change.id == crt_cb_) {
		get_app().set_crt_enabled(change.checked);
	} else if(change.id == scan_lines_cb_) {
		get_app().set_scan_lines_enabled(change.checked);
	} else if(change.id == color_bleed_cb_) {
		get_app().set_color_bleed_enabled(change.checked);
	}

	return true;
}

} // namespace pxe