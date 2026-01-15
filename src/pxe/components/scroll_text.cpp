// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/component.hpp>
#include <pxe/components/scroll_text.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <algorithm>
#include <optional>
#include <raygui.h>
#include <sstream>
#include <string>

namespace pxe {
class app;

auto scroll_text::init(app &app) -> result<> {
	if(const auto err = ui_component::init(app).unwrap(); err) {
		return error("failed to initialize base UI component", *err);
	}
	return true;
}

auto scroll_text::update(const float delta) -> result<> {
	// Let base UI component run its update behavior first.
	if(const auto err = ui_component::update(delta).unwrap(); err) {
		return error("failed to update base UI component", *err);
	}

	return true;
}

auto scroll_text::draw() -> result<> {
	// Allow base to perform any draw-side work first.
	if(const auto err = ui_component::draw().unwrap(); err) {
		return error("failed to draw base UI component", *err);
	}

	if(!is_visible()) {
		return true;
	}

	if(is_enabled()) {
		GuiEnable();
	} else {
		GuiDisable();
	}

	GuiSetFont(get_font());
	GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(get_font_size()));

	auto [x, y] = get_position();
	const auto [width, height] = get_size();
	const auto bound = Rectangle{.x = x, .y = y, .width = width, .height = height};
	GuiScrollPanel(bound, title_.c_str(), content_, &scroll_, &view_);

	BeginScissorMode(static_cast<int>(view_.x),
					 static_cast<int>(view_.y),
					 static_cast<int>(view_.width),
					 static_cast<int>(view_.height));

	auto start_y = view_.y + scroll_.y;
	auto const start_x = view_.x + scroll_.x;

	for(const auto &line: text_lines_) {
		DrawTextEx(get_font(), line.c_str(), {.x = start_x, .y = start_y}, get_font_size(), spacing_, BLACK);
		const auto [_, line_y] = MeasureTextEx(get_font(), line.c_str(), get_font_size(), spacing_);
		start_y += line_y + line_spacing_;
	}

	EndScissorMode();

	return true;
}

auto scroll_text::set_text(const std::string &text) -> void {
	float max_x = 0;
	float total_height = 0;

	std::istringstream stream(text);
	std::string line;
	while(std::getline(stream, line)) {
		text_lines_.emplace_back(line);

		const auto [x, y] = MeasureTextEx(get_font(), line.c_str(), get_font_size(), spacing_);
		max_x = std::max(x, max_x);
		total_height += y + line_spacing_;
	}

	content_.x = 0;
	content_.y = 0;
	content_.width = max_x;
	content_.height = total_height;
	scroll_ = {.x = 0, .y = 0};
	view_ = {.x = 0, .y = 0, .width = 0, .height = 0};
}

void scroll_text::set_font_size(const float &font_size) {
	ui_component::set_font_size(font_size);
	line_spacing_ = font_size * 0.5F;
	spacing_ = font_size * 0.2F;
}

} // namespace pxe