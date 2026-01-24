// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <cstddef>

namespace pxe {
class app;

class banner: public scene {
public:
	banner() = default;
	~banner() override = default;

	// Copyable
	banner(const banner &) = default;
	auto operator=(const banner &) -> banner & = default;

	// Movable
	banner(banner &&) noexcept = default;
	auto operator=(banner &&) noexcept -> banner & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto layout(size screen_size) -> result<> override;
	[[nodiscard]] auto update(float delta) -> result<> override;

	struct finished {};

private:
	static constexpr auto sprite_sheet_name = "menu";
	static constexpr auto logo_frame = "pxe.png";

	static constexpr auto time_to_show = 5.0F;
	float total_time_{0.0F};

	size_t logo_{0};
};

} // namespace pxe