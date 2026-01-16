// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <string>

namespace pxe {
class app;

class ui_component: public component {
public:
	ui_component() = default;
	~ui_component() override = default;

	// Copyable
	ui_component(const ui_component &) = default;
	auto operator=(const ui_component &) -> ui_component & = default;

	// Non-movable
	ui_component(ui_component &&) noexcept = default;
	auto operator=(ui_component &&) noexcept -> ui_component & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;

	auto set_font(const Font &font) -> void {
		font_ = font;
	}

	[[nodiscard]] auto get_font() const -> Font {
		return font_;
	}

	virtual auto set_font_size(const float &size) -> void {
		font_size_ = size;
	}

	[[nodiscard]] auto get_font_size() const -> float {
		return font_size_;
	}

	auto set_click_sfx(const std::string &sound_name) -> void {
		click_sfx_ = sound_name;
	}

	[[nodiscard]] auto play_click_sfx() -> result<>;

private:
	Font font_{};
	float font_size_ = 20.0F;
	std::string click_sfx_{"click"};
};

} // namespace pxe
