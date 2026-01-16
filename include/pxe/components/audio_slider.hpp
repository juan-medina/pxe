// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <cstddef>
#include <string>

namespace pxe {
class app;

class audio_slider: public ui_component {
public:
	[[nodiscard]] auto init(app &app) -> result<> override;

	[[nodiscard]] auto draw() -> result<> override;

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

	auto set_value(const float value) -> void {
		current_ = value;
	}

	auto set_muted(const bool muted) -> void {
		muted_ = muted;
	}

	struct audio_slider_changed {
		size_t id;
		float value;
		bool muted;
	};

private:
	std::string label_;
	size_t label_width_{100};
	size_t slider_width_{200};
	float min_{0.0F};
	float max_{100.0F};
	float current_{0.0F};
	bool muted_{false};
	float gap_slider_check_{0.0F};
	float line_height_{0.0F};

	auto calculate_size() -> void;
};

} // namespace pxe