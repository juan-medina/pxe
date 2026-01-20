// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <cstddef>
#include <format>
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

	auto set_controller_button(int button) -> void;
	auto set_controller_button_alignment(const vertical_alignment vertical, const horizontal_alignment horizontal)
		-> void {
		vertical_alignment_ = vertical;
		horizontal_alignment_ = horizontal;
	}

	static auto get_controller_button_name(const int button) -> std::string {
		return std::format("button_{:02}.png", button);
	}

	static auto constexpr controller_sprite_list() -> const char * {
		return "menu";
	}

private:
	std::string text_{"Button"};
	int game_pad_button_{-1};
	[[nodiscard]] auto do_click() -> result<>;
	static constexpr auto buttons_sprite_list = "menu";
	std::string button_sprite_;
	vertical_alignment vertical_alignment_{vertical_alignment::bottom};
	horizontal_alignment horizontal_alignment_{horizontal_alignment::right};
};

} // namespace pxe