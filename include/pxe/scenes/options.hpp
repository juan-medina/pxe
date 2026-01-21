// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/audio_slider.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/checkbox.hpp>
#include <pxe/components/component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <cstddef>
#include <vector>

namespace pxe {

class options: public scene {
public:
	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;
	[[nodiscard]] auto layout(size screen_size) -> result<> override;
	[[nodiscard]] auto update(float delta) -> result<> override;

	struct options_closed {};

	[[nodiscard]] auto show() -> result<> override;

private:
	static auto constexpr window_width = 300;
	static auto constexpr window_height = 150;
	static auto constexpr audio_label_width = 40;
	static auto constexpr audio_slider_width = 140;

	Color bg_color_ = {.r = 0x00, .g = 0x00, .b = 0x00, .a = 0x7F}; // #0000007F (50% transparent black)

	float screen_width_ = 0.0F;
	float screen_height_ = 0.0F;

	static constexpr auto control_row_gap = 5.0F;

	size_t window_ = 0;
	int close_window_ = 0;
	auto on_close_window() -> result<>;

	size_t music_slider_{0};
	size_t sfx_slider_{0};

	int slider_change_{0};
	auto on_slider_change(const audio_slider::audio_slider_changed &change) -> result<>;
	[[nodiscard]] auto set_slider_values(size_t slider, float value, bool muted) const -> result<>;

	size_t crt_cb_{0};
	size_t scan_lines_cb_{0};
	size_t color_bleed_cb_{0};

	[[nodiscard]] auto set_checkbox_value(size_t cb, bool value) const -> result<>;

	int checkbox_changed_{0};
	auto on_checkbox_changed(const checkbox::checkbox_changed &change) -> result<>;

	int button_click_{0};
	auto on_button_click(const button::click &click) -> result<>;

	size_t back_button_{0};
	size_t quit_button_{0};

	std::vector<size_t> ui_components_;

	[[nodiscard]] auto get_focus() const -> result<size_t>;
	[[nodiscard]] auto set_focus(size_t id) const -> result<>;
	[[nodiscard]] auto move_focus(size_t focus, direction dir) -> result<>;

	static constexpr auto click_sound = "click";
};

} // namespace pxe