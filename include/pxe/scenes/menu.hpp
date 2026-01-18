// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>
#include <cstddef>

namespace pxe {
class app;
struct size;

class menu: public scene {
public:
	menu() = default;
	~menu() override = default;

	// Copyable
	menu(const menu &) = default;
	auto operator=(const menu &) -> menu & = default;

	// Movable
	menu(menu &&) noexcept = default;
	auto operator=(menu &&) noexcept -> menu & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;

	auto layout(size screen_size) -> result<> override;

	struct go_to_game {};

	auto show() -> result<> override;

private:
	static constexpr auto sprite_sheet_name = "menu";
	static constexpr auto sprite_sheet_path = "resources/sprites/menu.json";
	static constexpr auto logo_sprite = "logo.png";

	size_t title_{0};
	static constexpr auto large_font_size = 20;
	static constexpr auto menu_music_path = "resources/music/menu.ogg";

	size_t play_button_{0};
	int button_click_{0};

	auto on_button_click(const button::click &evt) -> result<>;
};

} // namespace pxe