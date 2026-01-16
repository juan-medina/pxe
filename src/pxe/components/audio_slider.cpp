// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include "pxe/app.hpp"
#include <pxe/components/audio_slider.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <raygui.h>
#include <string>

namespace pxe {

auto audio_slider::init(app &app) -> result<> {
	if(const auto err = ui_component::init(app).unwrap(); err) {
		return error("failed to initialize base ui component", *err);
	}

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

	auto current_value = current_;
	auto current_muted = muted_;

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

	const auto value_str = std::to_string(static_cast<int>(current_)) + " %";

	x += static_cast<float>(label_width_);

	if(is_enabled()) {
		if(muted_) {
			GuiDisable();
		}
	}
	GuiSlider({.x = x, .y = y, .width = static_cast<float>(slider_width_), .height = line_height_},
			  "",
			  value_str.c_str(),
			  &current_,
			  min_,
			  max_);

	if(is_enabled()) {
		GuiEnable();
	}

	x += static_cast<float>(slider_width_) + gap_slider_check_;

	GuiCheckBox({.x = x, .y = y, .width = line_height_, .height = line_height_}, "muted", &muted_);

	GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, default_text_color);

	if(current_muted != muted_ || current_value != current_) {
		if(const auto err = play_click_sound().unwrap(); err) {
			return error("failed to play click sound", *err);
		}
		get_app().post_event(audio_slider_changed{.id = get_id(), .value = current_, .muted = muted_});
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

} // namespace pxe