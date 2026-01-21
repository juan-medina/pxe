// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/button.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <cstddef>
#include <string>

namespace pxe {
class app;

class audio_slider: public ui_component {
public:
	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;
	[[nodiscard]] auto update(float delta) -> result<> override;

	auto set_label(const std::string &label) -> void {
		label_ = label;
	}

	auto set_label_width(const size_t width) -> void {
		label_width_ = width;
		calculate_size();
	}

	auto set_slider_width(const size_t width) -> void {
		slider_width_ = width;
		calculate_size();
	}

	auto set_value(const size_t value) -> void {
		current_ = value;
		internal_current_ = static_cast<float>(value);
	}

	auto set_muted(const bool muted) -> void {
		muted_ = muted;
	}

	struct audio_slider_changed {
		size_t id;
		size_t value;
		bool muted;
	};

private:
	std::string label_;
	size_t label_width_{100};
	size_t slider_width_{200};
	float internal_min_{0.0F};
	float internal_max_{100.0F};
	float internal_current_{0.0F};
	size_t current_{0};
	bool muted_{false};
	float gap_slider_check_{0.0F};
	float line_height_{0.0F};

	float acceleration_timer_{0.0F};
	static constexpr float min_speed = 25.0F;
	static constexpr float max_speed = 200.0F;
	static constexpr float acceleration_time = 1.0F;

	static auto constexpr controller_button = GAMEPAD_BUTTON_RIGHT_FACE_LEFT;
	static auto constexpr button_sheet = button::controller_sprite_list();
	std::string button_frame_;

	auto calculate_size() -> void;

	[[nodiscard]] auto send_event() -> result<>;
};

} // namespace pxe