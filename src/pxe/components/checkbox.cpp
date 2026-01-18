// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/checkbox.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <raygui.h>

namespace pxe {

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

	auto current_value = checked_;

	GuiSetFont(get_font());
	GuiSetStyle(DEFAULT, TEXT_SIZE, static_cast<int>(get_font_size()));

	const auto [x, y] = get_position();

	GuiCheckBox({.x = x, .y = y, .width = check_box_size_, .height = check_box_size_}, title_.c_str(), &checked_);

	if(current_value != checked_) {
		get_app().post_event(checkbox_changed{.id = get_id(), .checked = checked_});
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

} // namespace pxe