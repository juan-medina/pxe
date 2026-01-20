// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/components/sprite.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <string>

namespace pxe {
class app;
} // namespace pxe

namespace pxe {

class sprite_button: public ui_component {
public:
	[[nodiscard]] auto init(app &app,
							const std::string &sprite_sheet,
							const std::string &frame,
							Color normal_color = normal,
							Color hover_color = hover) -> result<>;
	[[nodiscard]] auto end() -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;
	[[nodiscard]] auto update(float delta) -> result<> override;

	auto set_scale(const float scale) -> void {
		sprite_.set_scale(scale);
	}

	auto set_controller_button(const int button) -> void {
		controller_button_ = button;
		if(button == -1) {
			controller_button_frame_.clear();
			return;
		}
		controller_button_frame_ = button::get_controller_button_name(button);
	}

	auto set_controller_button_alignment(vertical_alignment v_align, horizontal_alignment h_align) -> void {
		controller_v_align_ = v_align;
		controller_h_align_ = h_align;
	}

	[[nodiscard]] auto get_size() const -> const size & override {
		return sprite_.get_size();
	}

	auto set_position(const Vector2 &pos) -> void override {
		ui_component::set_position(pos);
		sprite_.set_position(pos);
	}

private:
	static constexpr auto normal_scale = 1.0F;
	static constexpr auto hover_scale = 1.2F;
	static constexpr auto normal = LIGHTGRAY;
	static constexpr auto hover = WHITE;

	Color normal_color_ = normal;
	Color hover_color_ = hover;

	sprite sprite_;
	static constexpr auto controller_button_sheet = button::controller_sprite_list();

	std::string sprite_sheet_;
	std::string frame_;
	std::string controller_button_frame_;

	bool hover_{false};
	int controller_button_{-1};

	vertical_alignment controller_v_align_{vertical_alignment::bottom};
	horizontal_alignment controller_h_align_{horizontal_alignment::right};

	[[nodiscard]] auto get_controller_button_position() const -> Vector2;
	[[nodiscard]] auto handle_click() -> result<>;
};

} // namespace pxe