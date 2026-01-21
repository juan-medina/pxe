// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include "pxe/app.hpp"
#include <pxe/components/audio_slider.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <algorithm>
#include <cstddef>
#include <raygui.h>
#include <string>

namespace pxe {

auto audio_slider::init(app &app) -> result<> {
	if(const auto err = ui_component::init(app).unwrap(); err) {
		return error("failed to initialize base ui component", *err);
	}

	button_frame_ = button::get_controller_button_name(controller_button);

	calculate_size();
	return true;
}

auto audio_slider::draw() -> result<> {
	if(auto const err = ui_component::draw().unwrap(); err) {
		return error("failed to draw base ui component", *err);
	}

	if(!is_visible()) {
		return true;
	}

	const auto current_value = current_;
	const auto current_muted = muted_;

	if(is_enabled()) {
		GuiEnable();
	} else {
		GuiDisable();
	}

	GuiSetFont(get_font());
	const auto default_text_color = GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL);

	GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(get_font_size()));
	GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(BLACK));

	auto [x, y] = get_position();

	GuiLabel({.x = x, .y = y, .width = static_cast<float>(label_width_), .height = line_height_}, label_.c_str());

	const auto value_str = std::to_string(static_cast<int>(internal_current_)) + " %";

	x += static_cast<float>(label_width_);

	if(is_enabled()) {
		if(muted_) {
			GuiDisable();
		}
	}

	if(is_focussed()) {
		GuiSetState(STATE_FOCUSED);
	}

	GuiSlider({.x = x, .y = y, .width = static_cast<float>(slider_width_), .height = line_height_},
			  "",
			  value_str.c_str(),
			  &internal_current_,
			  internal_min_,
			  internal_max_);

	if(is_enabled()) {
		GuiEnable();
	}

	x += static_cast<float>(slider_width_) + gap_slider_check_;

	GuiCheckBox({.x = x, .y = y, .width = line_height_, .height = line_height_}, "muted", &muted_);

	GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, default_text_color);

	current_ = static_cast<size_t>(internal_current_);

	if(current_muted != muted_ || current_value != current_) {
		if(const auto err = send_event().unwrap(); err) {
			return error("failed to send audio slider event", *err);
		}
	}

	if(is_focussed()) {
		GuiSetState(STATE_NORMAL);

		const auto [cx, cy] = get_position();
		const auto [cw, ch] = get_size();
		if(const auto err =
			   get_app().draw_sprite(button_sheet, button_frame_, {.x = cx + cw + 10, .y = y + (ch / 2)}).unwrap();
		   err) {
			return error("failed to draw controller button sprite", *err);
		}
	}

	return true;
}

auto audio_slider::update(float delta) -> result<> {
	if(const auto err = ui_component::update(delta).unwrap(); err) {
		return error("failed to update base ui component", *err);
	}

	if(is_focussed()) {
		const auto &app = get_app();
		const bool moving_left = app.is_direction_down(direction::left);
		const bool moving_right = app.is_direction_down(direction::right);

		if(moving_left || moving_right) {
			acceleration_timer_ += delta;
			const float acceleration_factor = std::min(acceleration_timer_ / acceleration_time, 1.0F);
			const float current_speed = min_speed + ((max_speed - min_speed) * acceleration_factor);
			const float amount = delta * current_speed;

			if(moving_left) {
				internal_current_ -= amount;
				internal_current_ = std::max(internal_current_, internal_min_);
			} else {
				internal_current_ += amount;
				internal_current_ = std::min(internal_current_, internal_max_);
			}
		} else {
			acceleration_timer_ = 0.0F;
		}

		if(app.is_controller_button_pressed(controller_button)) {
			muted_ = !muted_;
			if(const auto err = send_event().unwrap(); err) {
				return error("failed to send audio slider event", *err);
			}
		}
	} else {
		acceleration_timer_ = 0.0F;
	}

	return true;
}

auto audio_slider::calculate_size() -> void {
	const auto [gap, line_height] = MeasureTextEx(get_font(), " 100 % ", get_font_size(), 1);
	const auto check_gap = MeasureTextEx(get_font(), " muted", get_font_size(), 1).x;

	gap_slider_check_ = gap;
	line_height_ = line_height;

	float total_width = 0.0F;
	total_width += static_cast<float>(label_width_);
	total_width += static_cast<float>(slider_width_);
	total_width += gap_slider_check_;
	total_width += line_height_ + check_gap;

	set_size({.width = total_width, .height = line_height_});
}

auto audio_slider::send_event() -> result<> {
	if(const auto err = play_click_sfx().unwrap(); err) {
		return error("failed to play click sfx", *err);
	}
	get_app().post_event(audio_slider_changed{.id = get_id(), .value = current_, .muted = muted_});
	return true;
}

} // namespace pxe