// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/components/version_display.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <algorithm>
#include <cassert>
#include <optional>
#include <string>

namespace pxe {
auto version_display::init(app &app) -> result<> {
	if(const auto err = ui_component::init(app).unwrap(); err) {
		return error("failed to initialize base UI component", *err);
	}

	auto [major, minor, patch, build] = app.get_version();
	static_assert(components_colors.size() == 8);
	assert(components_colors.size() == parts_.size());
	parts_ = {
		part{.text = std::string("v"), .color = components_colors.at(0), .offset = 0.0F},
		part{.text = std::to_string(major), .color = components_colors.at(1), .offset = 0.0F},
		part{.text = std::string("."), .color = components_colors.at(2), .offset = 0.0F},
		part{.text = std::to_string(minor), .color = components_colors.at(3), .offset = 0.0F},
		part{.text = std::string("."), .color = components_colors.at(4), .offset = 0.0F},
		part{.text = std::to_string(patch), .color = components_colors.at(5), .offset = 0.0F},
		part{.text = std::string("."), .color = components_colors.at(6), .offset = 0.0F},
		part{.text = std::to_string(build), .color = components_colors.at(7), .offset = 0.0F},
	};

	// Calculate total width and height
	float width = 0;
	float height = 0;

	for(auto &part: parts_) {
		const auto [size_x, size_y] = MeasureTextEx(get_font(), part.text.c_str(), get_font_size(), 1.0F);
		part.offset = width;
		width += size_x + parts_spacing_;
		height = std::max(size_y, height);
	}

	set_size({.width = width, .height = height});

	return true;
}

auto version_display::end() -> result<> {
	parts_ = {};
	return ui_component::end();
}

auto version_display::update(const float delta) -> result<> {
	if(const auto err = ui_component::update(delta).unwrap(); err) {
		return error("failed to update base UI component", *err);
	}

	const auto inside = point_inside(GetMousePosition());

	if(hover_ && !inside) {
		SetMouseCursor(MOUSE_CURSOR_DEFAULT);
	}

	hover_ = false;
	if(inside) {
		hover_ = true;
		SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
		if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
			if(const auto err = play_click_sound().unwrap(); err) {
				return error("failed to play click sound", *err);
			}
			get_app().post_event(button::click{get_id()});
		}
	}
	return true;
}

auto version_display::draw() -> result<> {
	if(const auto err = ui_component::draw().unwrap(); err) {
		return error("failed to draw base UI component", *err);
	}

	if(!is_visible()) {
		return true;
	}

	const auto pos = get_position();
	draw_parts({.x = pos.x + shadow_offset_, .y = pos.y + shadow_offset_}, true);
	draw_parts(pos, false);

	return true;
}

auto version_display::draw_parts(const Vector2 pos, const bool shadow) -> void {
	auto part_pos = pos;
	for(const auto &[text, color, offset]: parts_) {
		part_pos.x = pos.x + offset;
		DrawTextEx(get_font(), text.c_str(), part_pos, get_font_size(), 1.0F, shadow ? BLACK : color);
	}
}

void version_display::set_font_size(const float &size) {
	ui_component::set_font_size(size);
	parts_spacing_ = size / 10.0F;
	shadow_offset_ = size / 10.0F * 2.0F;
}

} // namespace pxe