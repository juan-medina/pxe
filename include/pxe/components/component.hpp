// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/result.hpp>

#include <raylib.h>

#include <cassert>
#include <cstddef>
#include <functional>
#include <optional>

namespace pxe {

class app;

struct size {
	float width{};
	float height{};
};

class component {
public:
	component(): id_(next_id++) {}
	virtual ~component() = default;

	// Non-copyable
	component(const component &) = default;
	auto operator=(const component &) -> component & = default;

	// Non-movable
	component(component &&) noexcept = default;
	auto operator=(component &&) noexcept -> component & = default;

	[[nodiscard]] virtual auto init(app &app) -> result<>;
	[[nodiscard]] virtual auto end() -> result<>;

	[[nodiscard]] virtual auto update(float /*delta*/) -> result<> {
		return true;
	}

	[[nodiscard]] virtual auto draw() -> result<> {
		return true;
	}

	virtual auto set_position(const Vector2 &pos) -> void {
		pos_ = pos;
	}

	[[nodiscard]] auto get_position() const -> const Vector2 & {
		return pos_;
	}

	auto set_size(const size &size) -> void {
		size_ = size;
	}

	[[nodiscard]] auto get_size() const -> const size & {
		return size_;
	}

	static auto point_inside(const Vector2 pos, const size size, const Vector2 point) -> bool {
		return CheckCollisionPointRec(point, {.x = pos.x, .y = pos.y, .width = size.width, .height = size.height});
	}

	[[nodiscard]] virtual auto point_inside(const Vector2 point) const -> bool {
		return point_inside(pos_, size_, point);
	}

	auto set_visible(const bool visible) -> void {
		visible_ = visible;
	}

	[[nodiscard]] auto is_visible() const -> bool {
		return visible_;
	}

	[[nodiscard]] auto get_id() const -> size_t {
		return id_;
	}

	[[nodiscard]] auto is_enabled() const -> bool {
		return enabled_;
	}

	auto set_enabled(const bool enabled) -> void {
		enabled_ = enabled;
	}

protected:
	[[nodiscard]] auto get_app() -> app & {
		assert(app_.has_value() && "app is not set");
		return app_->get();
	}

	[[nodiscard]] auto get_app() const -> const app & {
		assert(app_.has_value() && "app is not set");
		return app_->get();
	}

private:
	std::optional<std::reference_wrapper<app>> app_;
	Vector2 pos_{};
	size size_{};
	bool visible_ = true;
	bool enabled_ = true;
	size_t id_{0};
	static size_t next_id;
};
} // namespace pxe