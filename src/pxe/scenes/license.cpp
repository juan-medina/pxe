// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/scroll_text.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/license.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <algorithm>
#include <format>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <string>

namespace pxe {

auto license::init(app &app) -> result<> {
	if(const auto err = scene::init(app).unwrap(); err) {
		return error("failed to initialize base component", *err);
	}

	SPDLOG_INFO("license scene initialized");

	if(const auto err = register_component<scroll_text>().unwrap(scroll_text_); err) {
		return error("failed to register scroll text component", *err);
	}

	char *text = nullptr;
	if(text = LoadFileText(license_path); text == nullptr) {
		return error(std::format("failed to load license file from {}", license_path));
	}

	std::shared_ptr<scroll_text> scroll_text_ptr;
	if(const auto err = get_component<scroll_text>(scroll_text_).unwrap(scroll_text_ptr); err) {
		UnloadFileText(text);
		return error("failed to get scroll text component", *err);
	}

	scroll_text_ptr->set_text(text);
	UnloadFileText(text);
	scroll_text_ptr->set_position({.x = 10, .y = 10});
	scroll_text_ptr->set_size({.width = 500, .height = 400});
	scroll_text_ptr->set_title("License");

	if(const auto err = register_component<button>().unwrap(accept_button_); err) {
		return error("failed to register accept button component", *err);
	}

	std::shared_ptr<button> button_ptr;
	if(const auto err = get_component<button>(accept_button_).unwrap(button_ptr); err) {
		return error("failed to get accept button component", *err);
	}
	button_ptr->set_text("Accept");
	button_ptr->set_position({.x = 0, .y = 0});
	button_ptr->set_size({.width = 60, .height = 30});

	button_click_ = app.bind_event<button::click>(this, &license::on_button_click);

	return true;
}

auto license::end() -> result<> {
	get_app().unsubscribe(button_click_);
	return scene::end();
}

auto license::layout(const size screen_size) -> result<> {
	std::shared_ptr<scroll_text> scroll_text_ptr;
	if(const auto err = get_component<scroll_text>(scroll_text_).unwrap(scroll_text_ptr); err) {
		return error("failed to get scroll text component", *err);
	}

	const auto min_width = screen_size.width * 2.5F / 3.0F;
	scroll_text_ptr->set_size({.width = std::min(min_width, 1200.0F), .height = screen_size.height * 3.5F / 5.0F});

	scroll_text_ptr->set_position({.x = (screen_size.width - scroll_text_ptr->get_size().width) / 2.0F,
								   .y = (screen_size.height - scroll_text_ptr->get_size().height) / 2.0F});

	std::shared_ptr<button> button_ptr;
	if(const auto err = get_component<button>(accept_button_).unwrap(button_ptr); err) {
		return error("failed to get accept button component", *err);
	}

	const auto [width, height] = button_ptr->get_size();
	float const button_x = (screen_size.width - width) / 2.0F;
	float const button_y = scroll_text_ptr->get_position().y + scroll_text_ptr->get_size().height + 10;
	button_ptr->set_position({.x = button_x, .y = button_y});

	return true;
}

auto license::on_button_click(const button::click &evt) -> result<> {
	if(evt.id == accept_button_) {
		get_app().post_event(accepted{});
	}
	return true;
}

} // namespace pxe