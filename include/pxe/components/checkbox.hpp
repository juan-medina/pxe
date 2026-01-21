// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/button.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <cstddef>
#include <string>

namespace pxe {
class app;

class checkbox: public ui_component {
public:
	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;
	[[nodiscard]] auto update(float delta) -> result<> override;

	auto set_title(const std::string &title) -> void;

	[[nodiscard]] auto get_title() const -> std::string {
		return title_;
	}

	auto set_checked(const bool checked) -> void {
		checked_ = checked;
	}

	[[nodiscard]] auto is_checked() const -> bool {
		return checked_;
	}

	struct checkbox_changed {
		size_t id{};
		bool checked{false};
	};

private:
	std::string title_;
	bool checked_{false};
	float check_box_size_{0.0F};

	static auto constexpr controller_button = GAMEPAD_BUTTON_RIGHT_FACE_DOWN;
	static auto constexpr button_sheet = button::controller_sprite_list();
	std::string button_frame_;

	[[nodiscard]] auto send_event() -> result<>;
};

} // namespace pxe