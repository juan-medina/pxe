// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-LicenseCopyrightText: MIT

#pragma once

#include <pxe/components/sprite.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace pxe {
class app;
class sprite;

class quick_bar: public ui_component {
public:
	quick_bar() = default;
	~quick_bar() override = default;

	// Copyable
	quick_bar(const quick_bar &) = default;
	auto operator=(const quick_bar &) -> quick_bar & = default;

	// Movable
	quick_bar(quick_bar &&) noexcept = default;
	auto operator=(quick_bar &&) noexcept -> quick_bar & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto init(app &app,
							const std::string &sprite_sheet,
							Color normal_color = LIGHTGRAY,
							Color hover_color = WHITE,
							float gap = 0.0F) -> result<>;
	auto set_position(const Vector2 &pos) -> void override;
	[[nodiscard]] auto end() -> result<> override;
	[[nodiscard]] auto update(float delta) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;

	[[nodiscard]] auto add_button(const std::string &frame_name) -> result<size_t>;

	[[nodiscard]] auto set_button_frame_name(size_t button, const std::string &frame_name) const -> result<>;

private:
	auto recalculate() -> void;
	auto recalculate_size() -> void;

	float gap_{0.0F};
	std::string sprite_sheet_;
	std::vector<std::shared_ptr<sprite>> sprites_;

	Color normal_color_{LIGHTGRAY};
	Color hover_color_{WHITE};
};

} // namespace pxe
