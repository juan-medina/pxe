// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include "pxe/app.hpp"
#include <pxe/components/ui_component.hpp>
#include <pxe/components/window.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <raygui.h>

namespace pxe {

auto window::draw() -> result<> {
	if(!is_visible()) {
		return true;
	}
	auto [width, height] = get_size();
	auto [x, y] = get_position();

	GuiSetFont(get_font());
	GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(get_font_size()));

	if(const auto value = GuiWindowBox({.x = x, .y = y, .width = width, .height = height}, title_.c_str());
	   value != 0) {
		get_app().post_event(close{});
		if(const auto err = play_click_sound().unwrap(); err) {
			return error("failed to play click sound on window close", *err);
		}
	}

	return ui_component::draw();
}

} // namespace pxe