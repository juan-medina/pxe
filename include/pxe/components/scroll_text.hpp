// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

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

	auto set_text(const std::string &text) -> void;

	auto set_title(const std::string &title) -> void {
		this->title_ = title;
	}

	auto set_font_size(const float &font_size) -> void override;

private:
	std::string title_;
	std::vector<std::string> text_lines_;

	Vector2 scroll_ = {.x = 0, .y = 0};
	Rectangle view_ = {.x = 0, .y = 0, .width = 0, .height = 0};
	Rectangle content_ = {.x = 0, .y = 0, .width = 0, .height = 0};

	float line_spacing_ = 0.0F;
	float spacing_ = 0.0F;
};

} // namespace pxe
