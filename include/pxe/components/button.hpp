// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace pxe {
class app;

class button: public ui_component {
public:
	button() = default;
	~button() override = default;

	// Copyable
	button(const button &) = default;
	auto operator=(const button &) -> button & = default;

	// Movable
	button(button &&) noexcept = default;
	auto operator=(button &&) noexcept -> button & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;

	[[nodiscard]] auto update(float delta) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;

	auto set_text(const std::string &text) -> void {
		text_ = text;
	}

	[[nodiscard]] auto get_text() const -> const std::string & {
		return text_;
	}

	struct click {
		size_t id{};
	};

	enum class controller_button_position : std::uint8_t { bottom_left, bottom_right, top_left, top_right };

	auto set_controller_button(int button) -> void;
	auto set_controller_button_position(controller_button_position pos) -> void {
		controller_button_pos_ = pos;
	}

private:
	std::string text_{"Button"};
	int game_pad_button_{-1};
	[[nodiscard]] auto do_click() -> result<>;
	static constexpr auto buttons_sprite_list = "menu";
	std::string button_sprite_{"button_06.png"};
	controller_button_position controller_button_pos_{controller_button_position::top_right};
};

} // namespace pxe