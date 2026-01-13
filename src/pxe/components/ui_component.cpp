// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <optional>

namespace pxe {

auto ui_component::init(app &app) -> result<> {
	if(const auto err = component::init(app).unwrap(); err) {
		return error("failed to initialize base component", *err);
	}

	set_font(app.get_default_font());
	set_font_size(static_cast<float>(app.get_default_font_size()));

	return true;
}

auto ui_component::play_click_sound() -> result<> {
	return get_app().play_sound(click_sound_);
}

} // namespace pxe