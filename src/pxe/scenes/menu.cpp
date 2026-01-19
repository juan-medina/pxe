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
	play_button->set_size({.width = 80, .height = 35});
	play_button->set_font_size(large_font_size);
	play_button->set_controller_button(GAMEPAD_BUTTON_RIGHT_FACE_DOWN);

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

	const auto [button_width, button_height] = play_button->get_size();
	play_button->set_position({
		.x = (screen_size.width - button_width) / 2.0F,
		.y = ((screen_size.height - button_height) / 2.0F) + title_height,
	});

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
	}
	return true;
}

} // namespace pxe