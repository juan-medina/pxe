// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/components/sprite.hpp>
#include <pxe/result.hpp>

#include <cassert>
#include <string>

namespace pxe {
class app;

class sprite_anim: public sprite {
public:
	sprite_anim();
	~sprite_anim() override;

	// Copyable
	sprite_anim(const sprite_anim &) = default;
	auto operator=(const sprite_anim &) -> sprite_anim & = default;

	// Movable
	sprite_anim(sprite_anim &&) noexcept = default;
	auto operator=(sprite_anim &&) noexcept -> sprite_anim & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto init(app &app, const std::string &sprite_sheet, const std::string &frame) -> result<> override;
	[[nodiscard]] auto
	init(app &app, const std::string &sprite_sheet, const std::string &pattern, int frames, float fps) -> result<>;
	[[nodiscard]] auto update(float delta) -> result<> override;

	auto reset() -> result<>;

	auto play() -> void;
	auto stop() -> void;

	auto set_auto_loop(const bool auto_loop) {
		auto_loop_ = auto_loop;
	}

private:
	bool running_{false};
	std::string frame_pattern_;
	int frames_{1};
	int current_frame_{1};
	float fps_{1.0F};
	float time_accum_{0.0F};
	bool auto_loop_{true};

	void update_frame_name();
};

} // namespace pxe
