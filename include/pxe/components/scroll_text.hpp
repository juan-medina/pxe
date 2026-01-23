// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <optional>
#include <regex>
#include <string>
#include <vector>

namespace pxe {
class app;

class scroll_text: public ui_component {
public:
	scroll_text() = default;
	~scroll_text() override = default;

	// Copyable
	scroll_text(const scroll_text &) = default;
	auto operator=(const scroll_text &) -> scroll_text & = default;

	// Movable
	scroll_text(scroll_text &&) noexcept = default;
	auto operator=(scroll_text &&) noexcept -> scroll_text & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto update(float delta) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;

	[[nodiscard]] auto set_text(const std::string &text) -> result<>;

	auto set_title(const std::string &title) -> void {
		this->title_ = title;
	}

	auto set_font_size(const float &font_size) -> void override;

private:
	struct text_segment {
		std::string text;
		std::optional<std::string> url;
		float x = 0.0F; // Relative x position within the line
		float width = 0.0F;
		float height = 0.0F;
		bool is_hovered = false;
	};

	struct text_line {
		std::vector<text_segment> segments;
		float y = 0.0F; // Relative y position from content start
		float height = 0.0F;
	};

	// Regex to match markdown links: [text](url)
	static inline const std::regex link_pattern{R"(\[([^\]]+)\]\(([^\)]+)\))"};

	// Regex to validate URLs: must be https, no query params (?), no fragments (#)
	static inline const std::regex url_pattern{R"(^https://[^?#]+$)"};

	std::string title_;
	std::vector<text_line> text_lines_;

	Vector2 scroll_ = {.x = 0, .y = 0};
	Rectangle view_ = {.x = 0, .y = 0, .width = 0, .height = 0};
	Rectangle content_ = {.x = 0, .y = 0, .width = 0, .height = 0};

	float line_spacing_ = 0.0F;
	float spacing_ = 0.0F;

	Vector2 velocity_ = {.x = 0.0F, .y = 0.0F};
	float acceleration_ = 1000.0F; // pixels per second²
	float max_speed_ = 600.0F;	   // pixels per second
	float deceleration_ = 2000.0F; // pixels per second²

	bool hover_link_ = false; // Track if mouse is hovering over a link

	auto calculate_acceleration(float delta) -> bool;
	auto calculate_deceleration(float delta) -> void;
	static auto validate_url(const std::string &url) -> bool;
	auto find_link_at_position(const Vector2 &mouse_pos) -> text_segment*;
	static auto get_segment_color(const text_segment &segment) -> Color;
	auto reset_hover_states() -> void;
	[[nodiscard]] auto handle_link_hover(text_segment *hovered_segment) -> result<>;
	auto handle_controller_scroll(float delta) -> void;
};

} // namespace pxe
