// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/component.hpp>
#include <pxe/components/scroll_text.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <algorithm>
#include <format>
#include <optional>
#include <raygui.h>
#include <regex>
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
	if(const auto err = ui_component::update(delta).unwrap(); err) {
		return error("failed to update base UI component", *err);
	}

	if(is_visible() && is_enabled()) {
		if(get_app().is_in_controller_mode()) {
			if(const auto any_direction = calculate_acceleration(delta); !any_direction) {
				calculate_deceleration(delta);
			}

			// Clamp to max speed
			velocity_.y = std::clamp(velocity_.y, -max_speed_, max_speed_);
			velocity_.x = std::clamp(velocity_.x, -max_speed_, max_speed_);

			// Apply velocity to scroll position
			scroll_.y += velocity_.y * delta;
			scroll_.x += velocity_.x * delta;
		}
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
		// Calculate absolute y position for this line
		const float line_y = start_y + line.y;

		// Skip lines that are outside the visible scroll area (vertically)
		if(line_y + line.height < view_.y || line_y > view_.y + view_.height) {
			continue;
		}

		// Draw all segments in this line
		for(const auto &segment: line.segments) {
			const Color text_color =
				segment.url.has_value() ? GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_FOCUSED)) : BLACK;

			// Calculate absolute position using stored relative position
			const float seg_x = start_x + segment.x;

			DrawTextEx(
				get_font(), segment.text.c_str(), {.x = seg_x, .y = line_y}, get_font_size(), spacing_, text_color);

			// Draw underline for links
			if(segment.url.has_value()) {
				const float underline_y = line_y + segment.height + 1.0F;
				DrawLineEx(
					{.x = seg_x, .y = underline_y}, {.x = seg_x + segment.width, .y = underline_y}, 1.0F, text_color);
			}
		}
	}

	EndScissorMode();

	return true;
}

auto scroll_text::set_text(const std::string &text) -> result<> {
	float max_x = 0;
	float total_height = 0;

	std::istringstream stream(text);
	std::string line;
	while(std::getline(stream, line)) {
		text_line new_line;

		// Parse the line and split it into segments
		std::string remaining = line;
		std::smatch match;

		while(std::regex_search(remaining, match, link_pattern)) {
			// Add text before the link as a segment
			if(match.position() > 0) {
				std::string before_text = remaining.substr(0, match.position());
				new_line.segments.push_back({.text = before_text, .url = std::nullopt});
			}

			// Validate and add the link text with URL stored
			std::string link_text = match[1].str();
			std::string link_url = match[2].str();

			if(!validate_url(link_url)) {
				return error(
					std::format("Invalid URL '{}': must be https, no query parameters, and well-formed", link_url));
			}

			new_line.segments.push_back({.text = link_text, .url = link_url});

			// Move to the text after the link
			remaining = match.suffix().str();
		}

		// Add any remaining text after the last link
		if(!remaining.empty()) {
			new_line.segments.push_back({.text = remaining, .url = std::nullopt});
		}

		// If no segments were added (empty line), add an empty segment
		if(new_line.segments.empty()) {
			new_line.segments.push_back({.text = "", .url = std::nullopt});
		}

		// Calculate positions and sizes for all segments in this line
		float current_x = 0.0F;
		float line_height = 0.0F;
		for(auto &segment: new_line.segments) {
			const auto [seg_width, seg_height] =
				MeasureTextEx(get_font(), segment.text.c_str(), get_font_size(), spacing_);
			segment.x = current_x;
			segment.width = seg_width;
			segment.height = seg_height;
			current_x += seg_width;
			line_height = std::max(line_height, seg_height);
		}

		// Store line position and height
		new_line.y = total_height;
		new_line.height = line_height;

		text_lines_.emplace_back(new_line);

		// Update content dimensions
		max_x = std::max(max_x, current_x);
		total_height += line_height + line_spacing_;
	}

	content_.x = 0;
	content_.y = 0;
	content_.width = max_x;
	content_.height = total_height;
	scroll_ = {.x = 0, .y = 0};
	view_ = {.x = 0, .y = 0, .width = 0, .height = 0};

	return true;
}

void scroll_text::set_font_size(const float &font_size) {
	ui_component::set_font_size(font_size);
	line_spacing_ = font_size * 0.5F;
	spacing_ = font_size * 0.2F;
}

auto scroll_text::calculate_acceleration(float delta) -> bool {
	bool any_direction = false;
	if(get_app().is_direction_down(direction::up)) {
		velocity_.y += acceleration_ * delta;
		any_direction = true;
	}
	if(get_app().is_direction_down(direction::down)) {
		velocity_.y -= acceleration_ * delta;
		any_direction = true;
	}
	if(get_app().is_direction_down(direction::left)) {
		velocity_.x += acceleration_ * delta;
		any_direction = true;
	}
	if(get_app().is_direction_down(direction::right)) {
		velocity_.x -= acceleration_ * delta;
		any_direction = true;
	}

	return any_direction;
}

auto scroll_text::calculate_deceleration(float delta) -> void {
	const float decel = deceleration_ * delta;
	if(velocity_.y > 0) {
		velocity_.y = std::max(0.0F, velocity_.y - decel);
	} else if(velocity_.y < 0) {
		velocity_.y = std::min(0.0F, velocity_.y + decel);
	}
	if(velocity_.x > 0) {
		velocity_.x = std::max(0.0F, velocity_.x - decel);
	} else if(velocity_.x < 0) {
		velocity_.x = std::min(0.0F, velocity_.x + decel);
	}
}

auto scroll_text::validate_url(const std::string &url) -> bool {
	return std::regex_match(url, url_pattern);
}

} // namespace pxe