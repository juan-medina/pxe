// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <format>
#include <optional>
#include <raygui.h>

namespace pxe {

auto button::init(app &app) -> result<> {
	if(const auto err = ui_component::init(app).unwrap(); err) {
		return error("failed to initialize base UI component", *err);
	}

	return true;
}

auto button::end() -> result<> {
	return ui_component::end();
}

auto button::update(const float delta) -> result<> {
	if(!is_visible()) {
		return true;
	}

	if(const auto err = ui_component::update(delta).unwrap(); err) {
		return error("failed to update base UI component", *err);
	}

	if(is_enabled()) {
		if(game_pad_button_ != -1 && IsGamepadButtonPressed(0, game_pad_button_)) {
			if(get_app().is_in_controller_mode()) {
				return do_click();
			}
		}
	}

	return true;
}

auto button::draw() -> result<> {
	if(const auto err = ui_component::draw().unwrap(); err) {
		return error("failed to draw base UI component", *err);
	}

	if(!is_visible()) {
		return true;
	}

	if(is_enabled()) {
		GuiEnable();
	} else {
		GuiDisable();
	}

	const auto [x, y] = get_position();
	const auto [width, height] = get_size();

	GuiSetFont(get_font());
	GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(get_font_size()));

	if(const Rectangle rect{.x = x, .y = y, .width = width, .height = height}; GuiButton(rect, text_.c_str())) {
		return do_click();
	}

	if(get_app().is_in_controller_mode() && is_enabled()) {
		if(!button_sprite_.empty()) {
			auto pos = get_position();
			const auto size = get_size();

			switch(vertical_alignment_) {
			case vertical_alignment::top: {
				break;
			}
			case vertical_alignment::bottom:
				pos.y += size.height;
				break;
			case vertical_alignment::center:
				pos.y += size.height / 2.0F;
				break;
			}

			switch(horizontal_alignment_) {
			case horizontal_alignment::left:
				break;
			case horizontal_alignment::center:
				pos.x += size.width / 2.0F;
				break;
			case horizontal_alignment::right:
				pos.x += size.width;
				break;
			}
			if(const auto err = get_app().draw_sprite(buttons_sprite_list, button_sprite_, pos).unwrap(); err) {
				return error("failed to draw button sprite", *err);
			}
		}
	}

	return true;
}

auto button::set_controller_button(const int button) -> void {
	game_pad_button_ = button;
	if(button != -1) {
		button_sprite_ = std::format("button_{:02}.png", button);
	} else {
		button_sprite_.clear();
	}
}

auto button::do_click() -> result<> {
	if(const auto err = play_click_sfx().unwrap(); err) {
		return error("failed to play click sfx", *err);
	}
	get_app().post_event(click{.id = get_id()});
	return true;
}

} // namespace pxe