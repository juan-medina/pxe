// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <string>

namespace pxe {
class app;

class label: public ui_component {
public:
	label() = default;
	~label() override = default;

	// Copyable
	label(const label &) = default;
	auto operator=(const label &) -> label & = default;

	// Movable
	label(label &&) noexcept = default;
	auto operator=(label &&) noexcept -> label & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;

	[[nodiscard]] auto update(float delta) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;

	auto set_text(const std::string &text) -> void;

	[[nodiscard]] auto get_text() const -> const std::string & {
		return text_;
	}

	auto set_font_size(const float &size) -> void override;

	auto set_centered(const bool centered) -> void {
		centered_ = centered;
	}

	auto set_text_color(const Color &color) -> void {
		text_color_ = color;
	}

private:
	std::string text_{"label"};
	bool centered_{false};
	Color text_color_{WHITE};

	auto calculate_size() -> void;
};

} // namespace pxe
