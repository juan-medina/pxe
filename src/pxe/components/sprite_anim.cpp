// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/sprite.hpp>
#include <pxe/components/sprite_anim.hpp>
#include <pxe/result.hpp>

#include <format>
#include <optional>
#include <string>

namespace pxe {
class app;

sprite_anim::sprite_anim() = default;
sprite_anim::~sprite_anim() = default;

auto sprite_anim::init(app &app) -> result<> {
	if(const auto err = sprite::init(app).unwrap(); err) {
		return error("failed to initialize base component", *err);
	}
	return error("sprite anim component requires additional parameters to be initialized");
}

auto sprite_anim::init(app &app, const std::string &sprite_sheet, const std::string &frame) -> result<> {
	if(const auto err = sprite::init(app, sprite_sheet, frame).unwrap(); err) {
		return error("failed to initialize base component", *err);
	}
	return error("sprite anim component requires additional parameters to be initialized");
}

auto sprite_anim::init(app &app,
					   const std::string &sprite_sheet,
					   const std::string &pattern,
					   const int frames,
					   const float fps) -> result<> {
	frame_pattern_ = pattern;
	frames_ = frames;
	fps_ = fps;
	current_frame_ = 1;
	time_accum_ = 0.0F;

	update_frame_name();
	return sprite::init(app, sprite_sheet, frame_name());
}

auto sprite_anim::update(const float delta) -> result<> {
	if(!is_visible() || !running_) {
		return sprite::update(delta);
	}
	time_accum_ += delta;
	if(const float frame_time = 1.0F / fps_; time_accum_ >= frame_time) {
		time_accum_ -= frame_time;
		current_frame_++;
		if(current_frame_ > frames_) {
			if(!auto_loop_) {
				stop();
				set_visible(false);
			}
			current_frame_ = 1;
		}
		update_frame_name();
		if(const auto err = sprite::init(get_app(), sprite_sheet_name(), frame_name()).unwrap(); err) {
			return error("failed to update frame", *err);
		}
	}

	return sprite::update(delta);
}
auto sprite_anim::reset() -> result<> {
	current_frame_ = 1;
	time_accum_ = 0.0F;
	update_frame_name();
	if(const auto error = sprite::init(get_app(), sprite_sheet_name(), frame_name()).unwrap(); error) {
		return pxe::error("failed to reset sprite animation", *error);
	}
	return true;
}

auto sprite_anim::play() -> void {
	running_ = true;
	reset();
}

auto sprite_anim::stop() -> void {
	running_ = false;
}

void sprite_anim::update_frame_name() {
	set_frame_name(std::vformat(frame_pattern_, std::make_format_args(current_frame_)));
}

} // namespace pxe
