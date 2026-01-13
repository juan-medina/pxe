// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <array>
#include <string>

namespace pxe {
class app;

class version_display: public ui_component {
public:
	version_display() = default;
	~version_display() override = default;

	// Copyable
	version_display(const version_display &) = default;
	auto operator=(const version_display &) -> version_display & = default;

	// Movable
	version_display(version_display &&) noexcept = default;
	auto operator=(version_display &&) noexcept -> version_display & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;

	[[nodiscard]] auto update(float delta) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;

	auto set_font_size(const float &size) -> void override;

private:
	struct part {
		std::string text;
		Color color;
		float offset;
	};

	static constexpr std::array components_colors = {
		Color{.r = 0xF0, .g = 0x00, .b = 0xF0, .a = 0xFF}, // v
		Color{.r = 0xFF, .g = 0x00, .b = 0x00, .a = 0xFF}, // major
		Color{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF}, // .
		Color{.r = 0xFF, .g = 0xA5, .b = 0x00, .a = 0xFF}, // minor
		Color{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF}, // .
		Color{.r = 0xFF, .g = 0xFF, .b = 0x00, .a = 0xFF}, // patch
		Color{.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF}, // .
		Color{.r = 0x00, .g = 0xFF, .b = 0x00, .a = 0xFF}  // build
	};

	std::array<part, 8> parts_{};

	float parts_spacing_ = 0.0F;
	float shadow_offset_ = 0.0F;

	auto draw_parts(Vector2 pos, bool shadow) -> void;
	bool hover_{false};
};
} // namespace pxe