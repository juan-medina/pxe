// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/sprite_button.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <string>

namespace pxe {

auto sprite_button::init(app &app,
						 const std::string &sprite_sheet,
						 const std::string &frame,
						 const Color normal_color,
						 const Color hover_color) -> result<> {
	if(const auto err = ui_component::init(app).unwrap(); err) {
		return error("failed to initialize base UI component", *err);
	}

	sprite_sheet_ = sprite_sheet;
	frame_ = frame;
	normal_color_ = normal_color;
	hover_color_ = hover_color;

	if(controller_button_ >= 0) {
		controller_button_frame_ = button::get_controller_button_name(controller_button_);
	}

	if(const auto err = sprite_.init(app, sprite_sheet_, frame_).unwrap(); err) {
		return error("failed to init sprite size", *err);
	}

	set_size(sprite_.get_size());

	return true;
}

auto sprite_button::end() -> result<> {
	if(const auto err = ui_component::end().unwrap(); err) {
		return error("failed to end base UI component", *err);
	}

	if(const auto err = sprite_.end().unwrap(); err) {
		return error("failed to end sprite", *err);
	}

	return true;
}

auto sprite_button::draw() -> result<> {
	if(!is_visible()) {
		return true;
	}

	sprite_.set_scale(hover_ ? hover_scale : normal_scale);
	sprite_.set_tint(hover_ ? hover_color_ : normal_color_);
	if(const auto err = sprite_.draw().unwrap(); err) {
		return error("failed to draw sprite button internal sprite", *err);
	}
	sprite_.set_scale(normal_scale);

	if(get_app().is_in_controller_mode() && is_enabled()) {
		if(!controller_button_frame_.empty()) {
			const auto button_pos = get_controller_button_position();
			if(const auto err =
				   get_app().draw_sprite(controller_button_sheet, controller_button_frame_, button_pos).unwrap();
			   err) {
				return error("failed to draw controller button sprite", *err);
			}
		}
	}

	return true;
}

auto sprite_button::update(const float delta) -> result<> {
	if(!is_visible()) {
		return true;
	}

	if(const auto err = ui_component::update(delta).unwrap(); err) {
		return error("failed to update base UI component", *err);
	}

	hover_ = false;

	if(is_enabled()) {
		if(get_app().is_in_controller_mode() && controller_button_ != -1) {
			if(get_app().is_controller_button_pressed(controller_button_)) {
				if(const auto err = handle_click().unwrap(); err) {
					return error("failed to handle controller click", *err);
				}
			}
		} else if(const auto mouse_pos = GetMousePosition(); sprite_.point_inside(mouse_pos)) {
			hover_ = true;
			if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
				if(const auto err = handle_click().unwrap(); err) {
					return error("failed to handle mouse click", *err);
				}
			}
		}
	}

	return true;
}

auto sprite_button::get_controller_button_position() const -> Vector2 {
	const auto pos = get_position();
	const auto [width, height] = get_size();

	auto button_pos = pos;

	// Vertical alignment
	switch(controller_v_align_) {
	case vertical_alignment::top:
		button_pos.y = pos.y - (height / 2.0F);
		break;
	case vertical_alignment::center:
		button_pos.y = pos.y;
		break;
	case vertical_alignment::bottom:
		button_pos.y = pos.y + (height / 2.0F);
		break;
	}

	// Horizontal alignment
	switch(controller_h_align_) {
	case horizontal_alignment::left:
		button_pos.x = pos.x - (width / 2.0F);
		break;
	case horizontal_alignment::center:
		button_pos.x = pos.x;
		break;
	case horizontal_alignment::right:
		button_pos.x = pos.x + (width / 2.0F);
		break;
	}

	return button_pos;
}

auto sprite_button::handle_click() -> result<> {
	if(const auto err = play_click_sfx().unwrap(); err) {
		return error("failed to play click sound", *err);
	}

	get_app().post_event(button::click{.id = get_id()});

	return true;
}

} // namespace pxe