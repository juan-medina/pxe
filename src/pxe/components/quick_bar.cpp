// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-LicenseCopyrightText: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/quick_bar.hpp>
#include <pxe/components/sprite.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <string>

namespace pxe {

result<> quick_bar::init(app &app) {
	return ui_component::init(app);
}

auto quick_bar::init(app &app,
					 const std::string &sprite_sheet,
					 const Color normal_color,
					 const Color hover_color,
					 const float gap) -> result<> {
	if(const auto err = init(app).unwrap(); err) {
		return error("failed to initialize base quick_bar component", *err);
	}

	sprite_sheet_ = sprite_sheet;
	gap_ = gap;
	normal_color_ = normal_color;
	hover_color_ = hover_color;

	return true;
}

auto quick_bar::add_button(const std::string &frame_name) -> result<size_t> {
	auto sprite_ptr = std::make_shared<sprite>();
	if(const auto err = sprite_ptr->init(get_app(), sprite_sheet_, frame_name).unwrap(); err) {
		return error("failed to initialize sprite in quick_bar", *err);
	}
	assert(sprite_ptr->get_pivot().x == 0.5F && sprite_ptr->get_pivot().y == 0.5F && "sprite pivot must be centered");

	sprite_ptr->set_tint(normal_color_);
	sprites_.emplace_back(sprite_ptr);
	recalculate();

	return sprite_ptr->get_id();
}

auto quick_bar::set_button_frame_name(size_t button, const std::string &frame_name) const -> result<> {
	for(auto &sprite_ptr: sprites_) {
		if(sprite_ptr->get_id() == button) {
			sprite_ptr->set_frame_name(frame_name);
			return true;
		}
	}
	return error(std::format("can not set button frame, button id not found in quick_bar: {}", button));
}

auto quick_bar::set_position(const Vector2 &pos) -> void {
	ui_component::set_position(pos);
	recalculate();
}

auto quick_bar::end() -> result<> {
	for(auto &sprite_ptr: sprites_) {
		if(const auto err = sprite_ptr->end().unwrap(); err) {
			return error("failed to end sprite in quick_bar", *err);
		}
	}
	sprites_.clear();

	return ui_component::end();
}

auto quick_bar::update(float delta) -> result<> {
	for(const auto &sprite_ptr: sprites_) {
		if(const auto err = sprite_ptr->update(delta).unwrap(); err) {
			return error("failed to update sprite in quick_bar", *err);
		}
	}

	const auto mouse = GetMousePosition();
	for(const auto &sprite_ptr: sprites_) {
		if(sprite_ptr->point_inside(mouse)) {
			sprite_ptr->set_tint(hover_color_);
			if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
				if(const auto err = play_click_sound().unwrap(); err) {
					return error("failed to play click sound", *err);
				}
				get_app().post_event(button::click{.id = sprite_ptr->get_id()});
			}

		} else {
			sprite_ptr->set_tint(normal_color_);
		}
	}

	return ui_component::update(delta);
}

auto quick_bar::draw() -> result<> {
	for(const auto &sprite_ptr: sprites_) {
		if(const auto err = sprite_ptr->draw().unwrap(); err) {
			return error("failed to draw sprite in quick_bar", *err);
		}
	}

	return ui_component::draw();
}

auto quick_bar::recalculate() -> void {
	recalculate_size();
	const auto [width, height] = get_size();
	const auto [x, y] = get_position();

	float pos_x = x - (width / 2);
	const auto pos_y = y;

	for(const auto &sprite_ptr: sprites_) {
		const auto [sprite_width, _] = sprite_ptr->get_size();
		// we draw the sprite centered
		pos_x += sprite_width / 2;
		sprite_ptr->set_position({.x = pos_x, .y = pos_y});
		pos_x += sprite_width / 2;
		pos_x += gap_;
	}
}

auto quick_bar::recalculate_size() -> void {
	float total_width = 0.0F;
	float max_height = 0.0F;

	for(const auto &sprite_ptr: sprites_) {
		const auto [width, height] = sprite_ptr->get_size();
		total_width += width + gap_;
		max_height = std::max(max_height, height);
	}

	if(!sprites_.empty() && total_width > 0.0F) {
		total_width -= gap_;
	}

	set_size({.width = total_width, .height = max_height});
}

} // namespace pxe
