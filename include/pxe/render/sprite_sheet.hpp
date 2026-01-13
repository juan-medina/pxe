// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/render/texture.hpp>
#include <pxe/result.hpp>
#include <raylib.h>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/json.hpp>
#include <filesystem>
#include <string>
#include <system_error>
#include <unordered_map>

namespace pxe {

class sprite_sheet {
public:
	explicit sprite_sheet() = default;
	virtual ~sprite_sheet() = default;

	// Copyable
	sprite_sheet(const sprite_sheet &) = default;
	auto operator=(const sprite_sheet &) -> sprite_sheet & = default;

	// Movable
	sprite_sheet(sprite_sheet &&) noexcept = default;
	auto operator=(sprite_sheet &&) noexcept -> sprite_sheet & = default;

	auto init(const std::string &path) -> result<>;
	auto end() -> result<>;
	[[nodiscard]] auto
	draw(const std::string &name, const Vector2 &pos, const float &scale, const Color &tint = WHITE) const -> result<>;

	[[nodiscard]] auto frame_size(const std::string &name) const -> result<size>;

	[[nodiscard]] auto frame_pivot(const std::string &name) const -> result<Vector2>;

private:
	struct frame {
		Rectangle origin{};
		Vector2 pivot{};
	};

	texture texture_;
	std::unordered_map<std::string, frame> frames_;

	auto parse_frames(const jsoncons::json &parser) -> result<>;
	auto parse_meta(const jsoncons::json &parser, const std::filesystem::path &base_path) -> result<>;
	auto get_frame_data(const std::string &name) const -> result<frame>;
};

} // namespace pxe