// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <cstddef>

namespace pxe {
class app;
struct size;

class game_overlay: public scene {
public:
	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;

	auto layout(size screen_size) -> result<> override;

	struct version_click {};
	struct options_click {};

private:
	size_t version_display_ = 0;
	size_t quick_bar_ = 0;

	static constexpr auto margin = 15.0F;
	static constexpr auto bar_gap = 15.0F;
	static constexpr auto sprite_sheet = "sprites";
	static constexpr auto normal = Color{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0x3C};
	static constexpr auto hover = Color{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0x7F};
	static constexpr auto gap = 5.0F;

	size_t close_button_ = 0;
	size_t toggle_fullscreen_button_ = 0;
	size_t options_button_ = 0;

	int button_click_{0};

	static constexpr auto fullscreen_frame = "larger.png";
	static constexpr auto windowed_frame = "smaller.png";

	auto on_button_click(const button::click &evt) -> result<>;
};

} // namespace pxe
