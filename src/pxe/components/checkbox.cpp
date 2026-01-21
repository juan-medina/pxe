// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/checkbox.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <raygui.h>

namespace pxe {

auto checkbox::init(app &app) -> result<> {
	if(const auto err = ui_component::init(app).unwrap(); err) {
		return error("failed to initialize base ui component", *err);
	}

	button_frame_ = button::get_controller_button_name(controller_button);

	return true;
}

auto checkbox::draw() -> result<> {
	if(!is_visible()) {
		return true;
	}

	if(const auto err = ui_component::draw().unwrap(); err) {
		return error("failed to draw checkbox ui component", *err);
	}

	if(!is_enabled()) {
		GuiDisable();
	} else {
		GuiEnable();
	}

	const auto current_value = checked_;

	GuiSetFont(get_font());
	GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(get_font_size()));

	const auto [x, y] = get_position();

	if(is_focussed()) {
		GuiSetState(STATE_FOCUSED);
	}

	GuiCheckBox({.x = x, .y = y, .width = check_box_size_, .height = check_box_size_}, title_.c_str(), &checked_);

	if(is_focussed()) {
		GuiSetState(STATE_NORMAL);

		const auto [cw, ch] = get_size();
		if(const auto err =
			   get_app().draw_sprite(button_sheet, button_frame_, {.x = x - 10, .y = y + (ch / 2)}).unwrap();
		   err) {
			return error("failed to draw controller button sprite", *err);
		}
	}

	if(current_value != checked_) {
		if(const auto err = send_event().unwrap(); err) {
			return error("failed to send checkbox changed event", *err);
		}
	}

	return true;
}

auto checkbox::update(const float delta) -> result<> {
	if(const auto err = ui_component::update(delta).unwrap(); err) {
		return error("failed to update base ui component", *err);
	}

	if(!is_visible()) {
		return true;
	}

	if(!is_enabled()) {
		return true;
	}

	if(is_focussed()) {
		if(get_app().is_controller_button_pressed(controller_button)) {
			if(get_app().is_in_controller_mode()) {
				checked_ = !checked_;
				if(const auto err = send_event().unwrap(); err) {
					return error("failed to send checkbox changed event", *err);
				}
			}
		}
	}

	return true;
}

auto checkbox::set_title(const std::string &title) -> void {
	title_ = title;

	// concat a space to title
	const auto [width, height] = MeasureTextEx(get_font(), title_.c_str(), get_font_size(), 1);
	set_size({.width = width + (height * 2), .height = height});
	check_box_size_ = height;
}

auto checkbox::send_event() -> result<> {
	if(const auto err = play_click_sfx().unwrap(); err) {
		return error("failed to play click sfx", *err);
	}
	get_app().post_event(checkbox_changed{.id = get_id(), .checked = checked_});
	return true;
}

} // namespace pxe