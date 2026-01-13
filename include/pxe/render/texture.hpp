// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/result.hpp>

#include <raylib.h>

#include <string>

namespace pxe {

class texture {
public:
	explicit texture() = default;
	virtual ~texture() = default;

	// Copyable
	texture(const texture &) = default;
	auto operator=(const texture &) -> texture & = default;

	// Movable
	texture(texture &&) noexcept = default;
	auto operator=(texture &&) noexcept -> texture & = default;

	[[nodiscard]] auto draw(const Vector2 &pos) const -> result<>;

	[[nodiscard]] auto draw(Rectangle origin, Rectangle dest, Color tint, float rotation, Vector2 center) const
		-> result<>;

	[[nodiscard]] virtual auto init(const std::string &path) -> result<>;
	[[nodiscard]] virtual auto end() -> result<>;

	[[nodiscard]] auto get_size() const -> size {
		return size_;
	}

private:
	size size_{.width = 0, .height = 0};
	Texture2D texture_{};
};

} // namespace pxe