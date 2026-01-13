// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/component.hpp>
#include <pxe/components/label.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <optional>
#include <raygui.h>

namespace pxe {
class app;

auto label::init(app &app) -> result<> {
	if(const auto err = ui_component::init(app).unwrap(); err) {
		return error("failed to initialize base UI component", *err);
	}

	return true;
}

auto label::end() -> result<> {
	return ui_component::end();
}

auto label::update(const float delta) -> result<> {
	if(const auto err = ui_component::update(delta).unwrap(); err) {
		return error("failed to update base UI component", *err);
	}

	return true;
}

auto label::draw() -> result<> {
	if(const auto err = ui_component::draw().unwrap(); err) {
		return error("failed to draw base UI component", *err);
	}

	if(!is_visible()) {
		return true;
	}

	auto [x, y] = get_position();
	const auto [width, height] = get_size();

	GuiSetFont(get_font());
	const auto default_text_color = GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL);

	GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(get_font_size()));
	GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));

	if(centered_) {
		x -= width / 2;
	}
	GuiLabel({.x = x, .y = y, .width = width, .height = height}, text_.c_str());

	GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, default_text_color);

	return true;
}

auto label::set_text(const std::string &text) -> void {
	text_ = text;
	calculate_size();
}
auto label::set_font_size(const float &size) -> void {
	ui_component::set_font_size(size);
	calculate_size();
}

auto label::calculate_size() -> void {
	const auto [x, y] = MeasureTextEx(get_font(), text_.c_str(), get_font_size(), 1.0F);
	set_size({.width = x, .height = y});
}

} // namespace pxe