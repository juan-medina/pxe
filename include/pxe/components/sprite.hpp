// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <string>

namespace pxe {
class app;

class sprite: public component {
public:
	sprite() = default;
	~sprite() override = default;

	// Copyable
	sprite(const sprite &) = default;
	auto operator=(const sprite &) -> sprite & = default;

	// Movable
	sprite(sprite &&) noexcept = default;
	auto operator=(sprite &&) noexcept -> sprite & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] virtual auto init(app &app, const std::string &sprite_sheet, const std::string &frame) -> result<>;
	[[nodiscard]] auto end() -> result<> override;

	[[nodiscard]] auto update(float delta) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;

	[[nodiscard]] auto get_pivot() const -> Vector2 {
		return pivot_;
	}

	auto set_tint(const Color &tint) -> void {
		tint_ = tint;
	}

	virtual auto set_scale(float scale) -> void;

	[[nodiscard]] auto get_scale() const -> float {
		return scale_;
	}

	[[nodiscard]] auto point_inside(Vector2 point) const -> bool override;

	auto set_frame_name(const std::string &frame_name) {
		frame_ = frame_name;
	}

protected:
	[[nodiscard]] auto frame_name() const -> std::string {
		return frame_;
	}
	[[nodiscard]] auto sprite_sheet_name() const -> std::string {
		return sprite_sheet_;
	}

private:
	Color tint_ = WHITE;
	std::string sprite_sheet_;
	std::string frame_;
	float scale_ = 1.0F;

	size original_size_;
	Vector2 pivot_{};
};
} // namespace pxe