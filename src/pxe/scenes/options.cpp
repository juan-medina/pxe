// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/audio_slider.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/checkbox.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/components/window.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/options.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <cstddef>
#include <cstdlib>
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

	if(const auto err = register_component<checkbox>().unwrap(fullscreen_cb_); err) {
		return error("failed to register fullscreen checkbox component", *err);
	}
	if(const auto err = get_component<checkbox>(fullscreen_cb_).unwrap(checkbox_component); err) {
		return error("failed to get fullscreen checkbox component", *err);
	}
	checkbox_component->set_title("Fullscreen");

	if(const auto err = register_component<button>().unwrap(back_button_); err) {
		return error("failed to register back button component", *err);
	}

	std::shared_ptr<button> back_button_ptr;
	if(const auto err = get_component<button>(back_button_).unwrap(back_button_ptr); err) {
		return error("failed to get back button component", *err);
	}
	back_button_ptr->set_text(GuiIconText(ICON_PLAYER_PREVIOUS, "Back"));
	back_button_ptr->set_size({.width = 55, .height = 20});
	back_button_ptr->set_controller_button(GAMEPAD_BUTTON_MIDDLE_RIGHT);

#ifndef __EMSCRIPTEN__
	if(const auto err = register_component<button>().unwrap(quit_button_); err) {
		return error("failed to register quit button component", *err);
	}

	std::shared_ptr<button> quit_button_ptr;
	if(const auto err = get_component<button>(quit_button_).unwrap(quit_button_ptr); err) {
		return error("failed to get quit button component", *err);
	}

	quit_button_ptr->set_text(GuiIconText(ICON_EXIT, "Quit"));
	quit_button_ptr->set_size({.width = 55, .height = 20});
	quit_button_ptr->set_controller_button(GAMEPAD_BUTTON_RIGHT_FACE_UP);
#endif

	checkbox_changed_ = app.bind_event<checkbox::checkbox_changed>(this, &options::on_checkbox_changed);
	button_click_ = app.bind_event<button::click>(this, &options::on_button_click);

	ui_components_.push_back(music_slider_);
	ui_components_.push_back(sfx_slider_);
	ui_components_.push_back(crt_cb_);
	ui_components_.push_back(scan_lines_cb_);
	ui_components_.push_back(color_bleed_cb_);
	ui_components_.push_back(fullscreen_cb_);

	return true;
}

auto options::end() -> result<> {
	get_app().unsubscribe(close_window_);
	get_app().unsubscribe(slider_change_);
	get_app().unsubscribe(checkbox_changed_);
	get_app().unsubscribe(button_click_);

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

	float control_x = (screen_width_ / 2) - (slider_width / 2);
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

	if(const auto err = get_component<checkbox>(fullscreen_cb_).unwrap(checkbox_component); err) {
		return error("failed to get fullscreen checkbox component", *err);
	}

	control_y += slider_height + control_row_gap;
	checkbox_component->set_position({.x = control_x, .y = control_y});

	std::shared_ptr<button> back_button_ptr;
	if(const auto err = get_component<button>(back_button_).unwrap(back_button_ptr); err) {
		return error("failed to get back button component", *err);
	}

	control_y += slider_height + control_row_gap;
	const auto [button_width, button_height] = back_button_ptr->get_size();

	const auto center = (screen_width_ / 2);
#ifndef __EMSCRIPTEN__
	control_x = center - button_width - control_row_gap;
#else
	control_x = center - button_width / 2;
#endif

	back_button_ptr->set_position({.x = control_x, .y = control_y});

#ifndef __EMSCRIPTEN__
	std::shared_ptr<button> quit_button_ptr;
	if(const auto err = get_component<button>(quit_button_).unwrap(quit_button_ptr); err) {
		return error("failed to get quit button component", *err);
	}

	control_x = center + control_row_gap;

	quit_button_ptr->set_position({.x = control_x, .y = control_y});
#endif

	return scene::layout(screen_size);
}

auto options::update(const float delta) -> result<> {
	if(const auto err = scene::update(delta).unwrap(); err) {
		return error("failed to update options scene", *err);
	}
	if(!is_enabled() || !is_visible()) {
		return true;
	}

	const auto &app = get_app();
	if(app.is_in_controller_mode()) {
		size_t focus = 0;
		if(const auto err = get_focus().unwrap(focus); err) {
			return error("failed to get focused component", *err);
		}
		if(focus == 0) {
			if(const auto err = set_focus(music_slider_).unwrap(); err) {
				return error("failed to set focus", *err);
			}
			focus = music_slider_;
		}
		const auto up = app.is_direction_pressed(direction::up);
		const auto down = app.is_direction_pressed(direction::down);
		if(up || down) {
			if(const auto err = move_focus(focus, up ? direction::up : direction::down).unwrap(); err) {
				return error("failed to move focus", *err);
			}
		}
	}

	return true;
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

	const auto fullscreen_enabled = get_app().is_fullscreen();
	if(const auto err = set_checkbox_value(fullscreen_cb_, fullscreen_enabled).unwrap(); err) {
		return error("failed to set fullscreen checkbox value", *err);
	}

	if(get_app().is_in_controller_mode()) {
		size_t focus = 0;
		if(const auto err = get_focus().unwrap(focus); err) {
			return error("failed to get focused component", *err);
		}
		if(focus == 0) {
			if(const auto err = set_focus(music_slider_).unwrap(); err) {
				return error("failed to set focus", *err);
			}
		}
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

auto options::set_slider_values(const size_t slider, const float value, const bool muted) const -> result<> {
	std::shared_ptr<audio_slider> music_slider_component;
	if(const auto err = get_component<audio_slider>(slider).unwrap(music_slider_component); err) {
		return error("failed to get music slider component", *err);
	}

	music_slider_component->set_value(static_cast<int>(value * 100.0F));
	music_slider_component->set_muted(muted);

	return true;
}

auto options::set_checkbox_value(const size_t cb, const bool value) const -> result<> {
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
	} else if(change.id == fullscreen_cb_) {
		get_app().toggle_fullscreen();
	}

	return true;
}

auto options::on_button_click(const button::click &click) -> result<> {
	if(click.id == back_button_) {
		get_app().post_event(options_closed{});
	} else if(click.id == quit_button_) {
		get_app().close();
	}
	return true;
}

auto options::get_focus() const -> result<size_t> {
	for(const auto &component_id: ui_components_) {
		std::shared_ptr<ui_component> ui_comp;
		if(const auto err = get_component<ui_component>(component_id).unwrap(ui_comp); err) {
			return error("failed to get ui component", *err);
		}

		if(ui_comp->is_focussed()) {
			return component_id;
		}
	}

	return 0;
}

auto options::set_focus(size_t id) const -> result<> {
	for(const auto &component_id: ui_components_) {
		std::shared_ptr<ui_component> ui_comp;
		if(const auto err = get_component<ui_component>(component_id).unwrap(ui_comp); err) {
			return error("failed to get ui component", *err);
		}
		if(component_id == id) {
			ui_comp->set_focussed(true);
		} else {
			ui_comp->set_focussed(false);
		}
	}

	return true;
}

auto options::move_focus(const size_t focus, const direction dir) -> result<> {
	std::shared_ptr<ui_component> focused_comp;
	if(const auto err = get_component<ui_component>(focus).unwrap(focused_comp); err) {
		return error("failed to get focused ui component", *err);
	}
	const auto [fx, fy] = focused_comp->get_position();
	size_t best_id = 0;
	float best_distance = -1.0F;
	for(const auto &component_id: ui_components_) {
		if(component_id == focus) {
			continue;
		}

		std::shared_ptr<ui_component> ui_comp;
		if(const auto err = get_component<ui_component>(component_id).unwrap(ui_comp); err) {
			return error("failed to get ui component", *err);
		}
		const auto [ux, uy] = ui_comp->get_position();

		const auto vertical_distance = uy - fy;
		if((dir == direction::up) && (vertical_distance >= 0)) {
			continue;
		}
		if((dir == direction::down) && (vertical_distance <= 0)) {
			continue;
		}

		const auto distance = std::abs(vertical_distance);
		if((best_distance < 0) || (distance < best_distance)) {
			best_distance = distance;
			best_id = component_id;
		}
	}

	if(best_id != 0) {
		if(const auto err = set_focus(best_id).unwrap(); err) {
			return error("failed to set focus to new component", *err);
		}
		if(const auto err = get_app().play_sfx(click_sound).unwrap(); err) {
			return error("failed to play click sound", *err);
		}
	}

	return true;
}

} // namespace pxe