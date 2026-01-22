// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/components/sprite.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/menu.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <memory>
#include <optional>
#include <raygui.h>
#include <spdlog/spdlog.h>

namespace pxe {

auto menu::init(app &app) -> result<> {
	if(const auto err = scene::init(app).unwrap(); err) {
		return error("failed to initialize base component", *err);
	}

	if(const auto err = register_component<button>().unwrap(play_button_); err) {
		return error("failed to register play button component", *err);
	}

	std::shared_ptr<button> play_button;
	if(const auto err = get_component<button>(play_button_).unwrap(play_button); err) {
		return error("failed to get play button component", *err);
	}

	play_button->set_text(GuiIconText(ICON_PLAYER_PLAY, "Play"));
	play_button->set_position({.x = 0, .y = 0});
	play_button->set_size({.width = play_button_size, .height = 35});
	play_button->set_font_size(large_font_size);
	play_button->set_controller_button(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

	if(const auto err = register_component<button>().unwrap(about_button_); err) {
		return error("failed to register about button component", *err);
	}

	std::shared_ptr<button> about_button;
	if(const auto err = get_component<button>(about_button_).unwrap(about_button); err) {
		return error("failed to get about button component", *err);
	}

	about_button->set_text(GuiIconText(ICON_INFO, "About"));
	about_button->set_position({.x = 0, .y = 0});
	about_button->set_controller_button(GAMEPAD_BUTTON_RIGHT_FACE_UP);
	about_button->set_size({.width = other_buttons_size, .height = 20});
#ifndef __EMSCRIPTEN__

	if(const auto err = register_component<button>().unwrap(quit_button_); err) {
		return error("failed to register quit button component", *err);
	}

	std::shared_ptr<button> quit_button;
	if(const auto err = get_component<button>(quit_button_).unwrap(quit_button); err) {
		return error("failed to get quit button component", *err);
	}

	quit_button->set_text(GuiIconText(ICON_EXIT, "Quit"));
	quit_button->set_position({.x = 0, .y = 0});
	quit_button->set_size({.width = other_buttons_size, .height = 20});
	quit_button->set_controller_button(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
#endif

	button_click_ = app.bind_event<button::click>(this, &menu::on_button_click);

	if(const auto err = app.load_sprite_sheet(sprite_sheet_name, sprite_sheet_path).unwrap(); err) {
		return error("failed to initialize sprite sheet", *err);
	}

	if(const auto err = register_component<sprite>(sprite_sheet_name, logo_sprite).unwrap(title_); err) {
		return error("failed to register title label", *err);
	}

	std::shared_ptr<sprite> title;
	if(const auto err = get_component<sprite>(title_).unwrap(title); err) {
		return error("failed to get title sprite component", *err);
	}

	SPDLOG_INFO("menu scene initialized");

	return true;
}

auto menu::end() -> result<> {
	get_app().unsubscribe(button_click_);
	if(const auto err = get_app().unload_sprite_sheet(sprite_sheet_name).unwrap(); err) {
		return error("failed to end sprite sheet", *err);
	}
	return scene::end();
}

auto menu::layout(const size screen_size) -> result<> {
	std::shared_ptr<sprite> title;
	if(const auto err = get_component<sprite>(title_).unwrap(title); err) {
		return error("failed to get title sprite component", *err);
	}

	const auto [tittle_width, title_height] = title->get_size();
	title->set_position({
		.x = screen_size.width / 2.0F,
		.y = screen_size.height / 2.0F,
	});

	std::shared_ptr<button> play_button;
	if(const auto err = get_component<button>(play_button_).unwrap(play_button); err) {
		return error("failed to get play button component", *err);
	}

	const auto play_button_y = title->get_position().y + (title_height / 2.0F) + 20.0F;

	const auto [button_width, button_height] = play_button->get_size();
	play_button->set_position({
		.x = (screen_size.width - button_width) / 2.0F,
		.y = play_button_y,
	});

	const auto [play_width, play_height] = play_button->get_size();

	std::shared_ptr<button> about_button;
	if(const auto err = get_component<button>(about_button_).unwrap(about_button); err) {
		return error("failed to get about button component", *err);
	}
	constexpr auto button_gap = 5.0F;
	const auto buttons_y = play_button_y + play_height + button_gap;
#ifndef __EMSCRIPTEN__
	const auto [about_button_width, about_button_height] = about_button->get_size();

	const auto center = screen_size.width / 2.0F;
	const auto buttons_x = center - about_button_width - (button_gap / 2.0F);

	about_button->set_position({
		.x = buttons_x,
		.y = buttons_y,
	});

	std::shared_ptr<button> quit_button;
	if(const auto err = get_component<button>(quit_button_).unwrap(quit_button); err) {
		return error("failed to get quit button component", *err);
	}
	quit_button->set_position({
		.x = buttons_x + about_button_width + button_gap,
		.y = buttons_y,
	});
#else
	about_button->set_position({
		.x = (screen_size.width - button_width) / 2.0F,
		.y = buttons_y,
	});
#endif

	return true;
}

auto menu::show() -> result<> {
	if(const auto err = scene::show().unwrap(); err) {
		return error("failed to enable base scene", *err);
	}

	if(const auto err = get_app().play_music("resources/music/menu.ogg").unwrap(); err) {
		return error(menu_music_path, *err);
	}

	return true;
}

auto menu::on_button_click(const button::click &evt) -> result<> {
	if(evt.id == play_button_) {
		get_app().post_event(go_to_game{});
	} else if(evt.id == about_button_) {
		get_app().post_event(show_about{});
#ifndef __EMSCRIPTEN__
	} else if(evt.id == quit_button_) {
		get_app().close();
#endif
	}
	return true;
}

} // namespace pxe