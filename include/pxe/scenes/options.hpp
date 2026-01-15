// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <cstddef>

namespace pxe {

class options: public scene {
public:
	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;
	[[nodiscard]] auto layout(size screen_size) -> result<> override;

private:
	static auto constexpr window_width = 400;
	static auto constexpr window_height = 300;
	Color bg_color_ = {.r = 0x00, .g = 0x00, .b = 0x00, .a = 0x7F}; // #0000007F (50% transparent black)

	float screen_width_ = 0.0F;
	float screen_height_ = 0.0F;

	size_t window_ = 0;
	int close_window_ = 0;
	auto on_close_window() -> result<>;
};

} // namespace pxe