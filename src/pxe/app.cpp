// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/app.hpp>
#include <pxe/components/component.hpp>
#include <pxe/events.hpp>
#include <pxe/render/sprite_sheet.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/about.hpp>
#include <pxe/scenes/game_overlay.hpp>
#include <pxe/scenes/license.hpp>
#include <pxe/scenes/menu.hpp>
#include <pxe/scenes/options.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <fstream>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/json_decoder.hpp>
#include <jsoncons/json_reader.hpp>
#include <memory>
#include <raygui.h>
#include <spdlog/common.h>
#include <spdlog/spdlog-inl.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#	include <shellapi.h>
#	include <cstdint>
#elif defined(__APPLE__) || defined(__linux__)
#	include <unistd.h>
#elif defined(__EMSCRIPTEN__)
#	include <emscripten/emscripten.h>
#	include <emscripten/val.h>
#endif

namespace pxe {

static const auto empty_format = "%v";
static const auto color_line_format = "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v %@";

// =============================================================================
// Lifecycle - Initialization
// =============================================================================

auto app::init() -> result<> {
	if(const auto err = parse_version(version_file_path).unwrap(version_); err) {
		return error("error parsing the version", *err);
	}

	SPDLOG_DEBUG("parsed version: {}.{}.{}.{}", version_.major, version_.minor, version_.patch, version_.build);

	if(const auto err = setup_log().unwrap(); err) {
		return error("error initializing the application", *err);
	}

	if(const auto err = settings_.init(team_, name_).unwrap(); err) {
		return error("error initializing settings", *err);
	}

	if(const auto err = load_settings().unwrap(); err) {
		return error("error loading settings", *err);
	}

	if(const auto err = init_audio().unwrap(); err) {
		return error("audio device could not be initialized", *err);
	}

	subscribe_to_builtin_events();

	SPDLOG_INFO("init application");

	if(const auto err = init_window().unwrap(); err) {
		return error("failed to initialize window", *err);
	}

	default_font_ = GetFontDefault();

	register_builtin_scenes();

	if(const auto err = init_crt_resources().unwrap(); err) {
		return error("failed to initialize CRT resources", *err);
	}

	return true;
}

auto app::init_scenes() -> result<> {
	return init_all_scenes();
}

auto app::end() -> result<> {
	if(const auto err = persist_settings().unwrap(); err) {
		return error("failed to save settings on end application", *err);
	}

	unsubscribe_from_builtin_events();

	if(const auto err = end_all_scenes().unwrap(); err) {
		return error("failed to end scenes", *err);
	}

	if(custom_default_font_) {
		SPDLOG_DEBUG("unloading custom default font");
		UnloadFont(default_font_);
	}

	if(const auto err = cleanup_audio_resources().unwrap(); err) {
		return error("failed to cleanup audio resources", *err);
	}

	if(const auto err = cleanup_sprite_sheets().unwrap(); err) {
		return error("failed to cleanup sprite sheets", *err);
	}

	if(const auto err = settings_.end().unwrap(); err) {
		return error("error ending settings", *err);
	}

	if(const auto err = cleanup_crt_resources().unwrap(); err) {
		return error("failed to cleanup CRT resources", *err);
	}

	cleanup_render_textures();

	return true;
}

// =============================================================================
// Lifecycle - Main Loop
// =============================================================================

auto app::run() -> result<> {
	if(const auto err = init().unwrap(); err) {
		return error("error init the application", *err);
	}

	if(const auto err = init_scenes().unwrap(); err) {
		return error("error init scenes", *err);
	}

#ifndef __EMSCRIPTEN__
	set_fullscreen(full_screen_);
#endif

	BeginDrawing();
	ClearBackground(clear_color_);
	EndDrawing();
	update_controller_mode(0.0F);
	reset_direction_states();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(
		[](void *arg) {
			auto *self = static_cast<app *>(arg);
			if(const auto err = self->main_loop().unwrap(); err) {
				SPDLOG_ERROR("error main_loop: {}", err->get_message());
				emscripten_cancel_main_loop();
			}
		},
		this,
		0,
		true);
	return true;
#else
	while(!should_exit_) {
		should_exit_ = should_exit_ || WindowShouldClose();
		if(const auto err = main_loop().unwrap(); err) {
			return error("error during main loop", *err);
		}
	}

	if(const auto err = end().unwrap(); err) {
		return error("error ending the application", *err);
	}

	SPDLOG_INFO("application ended");
	return true;
#endif
}

auto app::main_loop() -> result<> {
	configure_gui_for_input_mode();

	if(const auto err = update().unwrap(); err) {
		return error("error updating the application", *err);
	}

	if(const auto err = draw().unwrap(); err) {
		return error("error drawing the application", *err);
	}

	return true;
}

auto app::update() -> result<> {
	if(size const screen_size = {.width = static_cast<float>(GetScreenWidth()),
								 .height = static_cast<float>(GetScreenHeight())};
	   screen_size_.width != screen_size.width || screen_size_.height != screen_size.height) {
		if(const auto err = screen_size_changed(screen_size).unwrap(); err) {
			return error("failed to handle screen size change", *err);
		}
	}

	const auto delta = GetFrameTime();

	update_scene_transition(delta);

	if(const auto err = update_all_scenes(delta).unwrap(); err) {
		return error("failed to update scenes", *err);
	}

	if(const auto err = event_bus_.dispatch().unwrap(); err) {
		return error("error dispatching events", *err);
	}

	if(const auto err = handle_escape_key().unwrap(); err) {
		return error("failed to handle escape key", *err);
	}

	update_music_stream();
	update_controller_mode(delta);
	reset_direction_states();

	return true;
}

auto app::draw() const -> result<> {
	if(const auto err = render_scenes_to_texture().unwrap(); err) {
		return error("failed to render scenes to texture", *err);
	}

	if(const auto err = apply_crt_shader().unwrap(); err) {
		return error("failed to apply CRT shader", *err);
	}

	if(const auto err = draw_final_output().unwrap(); err) {
		return error("failed to draw final output", *err);
	}

	return true;
}

auto app::close() -> void {
	should_exit_ = true;
}

// =============================================================================
// Scene Management - Core
// =============================================================================

auto app::find_scene_info(const scene_id id) -> result<std::shared_ptr<scene_info>> {
	for(auto &scene_info: scenes_) {
		if(scene_info->id == id) {
			return scene_info;
		}
	}
	return error(std::format("scene with id {} not found", id));
}

auto app::sort_scenes() -> void {
	std::ranges::sort(scenes_,
					  [](const auto &scene_a, const auto &scene_b) -> bool { return scene_a->layer < scene_b->layer; });
}

auto app::unregister_scene(const scene_id id) -> result<> {
	const auto it = std::ranges::find_if(scenes_, [id](const auto &scene) -> bool { return scene->id == id; });
	if(it != scenes_.end()) {
		if((*it)->scene_ptr) {
			if(const auto err = (*it)->scene_ptr->end().unwrap(); err) {
				return error(std::format("error ending scene with id: {} name: {}", id, (*it)->name), *err);
			}
			(*it)->scene_ptr.reset(nullptr);
		}
		scenes_.erase(it);
		return true;
	}
	return error(std::format("scene with id {} not found", id));
}

auto app::show_scene(const scene_id id, const bool show) -> result<> {
	std::shared_ptr<scene_info> info;
	if(const auto err = find_scene_info(id).unwrap(info); err) {
		return error(std::format("scene with id {} not found", id));
	}

	info->scene_ptr->set_visible(show);

	if(show) {
		if(const auto enable_err = info->scene_ptr->show().unwrap(); enable_err) {
			return error(std::format("failed to show scene with id: {} name: {}", id, info->name), *enable_err);
		}
		SPDLOG_DEBUG("show scene with id: {} name: {}", id, info->name);
	} else {
		if(const auto disable_err = info->scene_ptr->hide().unwrap(); disable_err) {
			return error(std::format("failed to hide scene with id: {} name: {}", id, info->name), *disable_err);
		}
		SPDLOG_DEBUG("hide scene with id: {} name: {}", id, info->name);
	}

	return true;
}

auto app::pause_scene(const scene_id id) -> result<> {
	std::shared_ptr<scene_info> info;
	if(const auto err = find_scene_info(id).unwrap(info); err) {
		return error(std::format("scene with id {} not found", id));
	}

	if(const auto pause_err = info->scene_ptr->pause().unwrap(); pause_err) {
		return error(std::format("failed to pause scene with id: {} name: {}", id, info->name), *pause_err);
	}
	SPDLOG_DEBUG("paused scene with id: {} name: {}", id, info->name);

	return true;
}

auto app::resume_scene(const scene_id id) -> result<> {
	std::shared_ptr<scene_info> info;
	if(const auto err = find_scene_info(id).unwrap(info); err) {
		return error(std::format("scene with id {} not found", id));
	}

	if(const auto resume_err = info->scene_ptr->resume().unwrap(); resume_err) {
		return error(std::format("failed to resume scene with id: {} name: {}", id, info->name), *resume_err);
	}
	SPDLOG_DEBUG("resumed scene with id: {} name: {}", id, info->name);

	return true;
}

auto app::replace_scene(const scene_id current_scene, const scene_id new_scene) -> result<> {
	start_scene_transition(current_scene, new_scene);
	return true;
}

auto app::reload_scene(const scene_id id) -> result<> {
	std::shared_ptr<scene_info> info;
	if(const auto err = find_scene_info(id).unwrap(info); err) {
		return error(std::format("scene with id {} not found", id));
	}

	if(const auto reset_err = info->scene_ptr->reset().unwrap(); reset_err) { // NOLINT(*-ambiguous-smartptr-reset-call)
		return error(std::format("failed to reset scene with id: {} name: {}", id, info->name), *reset_err);
	}
	SPDLOG_DEBUG("reset scene with id: {} name: {}", id, info->name);

	if(const auto layout_err = info->scene_ptr->layout(drawing_resolution_).unwrap(); layout_err) {
		return error(std::format("failed to layout scene with id: {} name: {}", id, info->name), *layout_err);
	}

	return true;
}

// =============================================================================
// Scene Management - Lifecycle
// =============================================================================

auto app::init_all_scenes() -> result<> {
	SPDLOG_INFO("init scenes");
	for(auto &info: scenes_) {
		if(const auto err = info->scene_ptr->init(*this).unwrap(); err) {
			return error(std::format("failed to initialize scene with id: {} name: {}", info->id, info->name), *err);
		}
		SPDLOG_DEBUG("initialized scene with id: {} name: {}", info->id, info->name);
	}
	return true;
}

auto app::end_all_scenes() -> result<> {
	SPDLOG_INFO("ending scenes");
	for(auto &info: scenes_) {
		if(info->scene_ptr) {
			if(const auto err = info->scene_ptr->end().unwrap(); err) {
				return error(std::format("error ending scene with id: {} name: {}", info->id, info->name), *err);
			}
			SPDLOG_DEBUG("end scene with id: {} name: {}", info->id, info->name);
			info->scene_ptr.reset(); // NOLINT(*-ambiguous-smartptr-reset-call)
		}
	}
	scenes_.clear();
	return true;
}

auto app::update_all_scenes(const float delta) const -> result<> {
	for(const auto &info: scenes_) {
		if(!info->scene_ptr->is_visible()) {
			continue;
		}
		if(const auto err = info->scene_ptr->update(delta).unwrap(); err) {
			return error(std::format("failed to update scene with id: {} name: {}", info->id, info->name), *err);
		}
	}
	return true;
}

auto app::draw_all_scenes() const -> result<> {
	for(const auto &info: scenes_) {
		if(!info->scene_ptr->is_visible()) {
			continue;
		}
		if(const auto err = info->scene_ptr->draw().unwrap(); err) {
			return error(std::format("failed to draw scene with id: {} name:", info->id, info->name), *err);
		}
	}
	return true;
}

auto app::layout_all_scenes() const -> result<> {
	for(const auto &scene_info: scenes_) {
		if(const auto err = scene_info->scene_ptr->layout(drawing_resolution_).unwrap(); err) {
			return error(std::format("failed to layout scene with id: {} name: {}", scene_info->id, scene_info->name),
						 *err);
		}
	}
	return true;
}

// =============================================================================
// Scene Management - Built-in
// =============================================================================

auto app::register_builtin_scenes() -> void {
	license_scene_ = register_scene<license>();
	menu_scene_ = register_scene<menu>(false);
	about_scene_ = register_scene<about>(false);
	register_scene<game_overlay>(999);
	options_scene_ = register_scene<options>(1000, false);
}

auto app::subscribe_to_builtin_events() -> void {
	version_click_ = on_event<game_overlay::version_click>(this, &app::on_version_click);
	options_click_ = on_event<game_overlay::options_click>(this, &app::on_options_click);
	options_closed_ = on_event<options::options_closed>(this, &app::on_options_closed);
	license_accepted_ = on_event<license::accepted>(this, &app::on_license_accepted);
	go_to_game_ = on_event<menu::go_to_game>(this, &app::on_go_to_game);
	back_to_menu_ = bind_event<back_to_menu_from>(this, &app::on_back_to_menu_from);
	show_about_ = on_event<menu::show_about>(this, &app::on_show_about);
	about_back_clicked_ = on_event<about::back_clicked>(this, &app::on_about_back_clicked);
}

auto app::unsubscribe_from_builtin_events() -> void {
	unsubscribe(version_click_);
	unsubscribe(options_click_);
	unsubscribe(options_closed_);
	unsubscribe(license_accepted_);
	unsubscribe(go_to_game_);
	unsubscribe(back_to_menu_);
	unsubscribe(show_about_);
	unsubscribe(about_back_clicked_);
}

// =============================================================================
// Scene Event Handlers
// =============================================================================

auto app::on_version_click() const -> result<> { // NOLINT(*-convert-member-functions-to-static)
	return open_url("https://github.com/juan-medina/energy-swap/releases");
}

auto app::on_options_click() -> result<> {
	if(const auto err = show_scene(options_scene_).unwrap(); err) {
		return error("failed to show options scene", *err);
	}

	for(const auto &info: scenes_) {
		if(info->id != options_scene_) {
			if(const auto err = info->scene_ptr->pause().unwrap(); err) {
				return error(std::format("failed to pause scene with id: {} ", info->id), *err);
			}
		}
	}

	return true;
}

auto app::on_options_closed() -> result<> {
	if(const auto err = persist_settings().unwrap(); err) {
		return error("failed to save settings on options close", *err);
	}

	if(const auto err = hide_scene(options_scene_).unwrap(); err) {
		return error("failed to hide options scene", *err);
	}

	for(const auto &info: scenes_) {
		if(info->id != options_scene_) {
			if(const auto err = info->scene_ptr->resume().unwrap(); err) {
				return error(std::format("failed to resume scene with id: {}", info->id), *err);
			}
		}
	}

	return true;
}

auto app::on_license_accepted() -> result<> {
	return replace_scene(license_scene_, menu_scene_);
}

auto app::on_go_to_game() -> result<> {
	return replace_scene(menu_scene_, main_scene_);
}

auto app::on_back_to_menu_from(back_to_menu_from from) -> result<> {
	return replace_scene(from.id, menu_scene_);
}

auto app::on_show_about() -> result<> {
	return replace_scene(menu_scene_, about_scene_);
}

auto app::on_about_back_clicked() -> result<> {
	return replace_scene(about_scene_, menu_scene_);
}

// =============================================================================
// Font Management
// =============================================================================

auto app::set_default_font(const std::string &path, const int size, const int texture_filter) -> result<> {
	if(std::ifstream const font_file(path); !font_file.is_open()) {
		return error(std::format("can not load  font file: {}", path));
	}

	if(custom_default_font_) {
		SPDLOG_DEBUG("unloading previous custom default font");
		UnloadFont(default_font_);
		custom_default_font_ = false;
	}

	auto font_size = size;
	const auto font = LoadFontEx(path.c_str(), font_size, nullptr, 0);
	if(font_size == 0) {
		font_size = font.baseSize;
	}

	set_default_font(font, font_size, texture_filter);
	custom_default_font_ = true;

	SPDLOG_DEBUG("set default font to {}", path);
	return true;
}

auto app::set_default_font(const Font &font, const int size, const int texture_filter) -> void {
	default_font_ = font;
	default_font_size_ = size;
	SetTextureFilter(font.texture, texture_filter);
	GuiSetFont(default_font_);
	GuiSetStyle(DEFAULT, TEXT_SIZE, size);
}

// =============================================================================
// Audio Management - System
// =============================================================================

auto app::init_audio() -> result<> {
	InitAudioDevice();
	if(IsAudioDeviceReady()) {
		audio_initialized_ = true;
		SPDLOG_INFO("audio device initialized");
		return true;
	}
	return error("failed to initialize audio device");
}

auto app::end_audio() -> result<> {
	if(audio_initialized_) {
		CloseAudioDevice();
		audio_initialized_ = false;
		SPDLOG_INFO("audio device closed");
		return true;
	}
	SPDLOG_WARN("audio device was not initialized");
	return true;
}

auto app::cleanup_audio_resources() -> result<> {
	for(const auto &[name, sfx]: sfx_) {
		if(IsSoundPlaying(sfx)) {
			StopSound(sfx);
			SPDLOG_DEBUG("stopped playing sfx {}", name);
		}
	}

	for(const auto &[name, sfx]: sfx_) {
		UnloadSound(sfx);
		SPDLOG_DEBUG("unloaded sfx {}", name);
	}
	sfx_.clear();

	if(music_playing_) {
		if(const auto err = stop_music().unwrap(); err) {
			return error("failed to stop music during app end", *err);
		}
	}

	return end_audio();
}

auto app::update_music_stream() const -> void {
	if(music_playing_) {
		UpdateMusicStream(background_music_);
	}
}

// =============================================================================
// Audio Management - SFX
// =============================================================================

auto app::load_sfx(const std::string &name, const std::string &path) -> result<> {
	if(std::ifstream const font_file(path); !font_file.is_open()) {
		return error(std::format("can not load sfx file: {}", path));
	}

	if(sfx_.contains(name)) {
		return error(std::format("sfx with name {} is already loaded", name));
	}

	Sound const sfx = LoadSound(path.c_str());
	if(!IsSoundValid(sfx)) {
		return error(std::format("sfx not valid from path: {}", path));
	}

	sfx_.emplace(name, sfx);
	SPDLOG_DEBUG("loaded sfx {} from {}", name, path);

	return true;
}

auto app::unload_sfx(const std::string &name) -> result<> {
	const auto it = sfx_.find(name);
	if(it == sfx_.end()) {
		return error(std::format("can't unload sfx with name {}, is not loaded", name));
	}

	UnloadSound(it->second);
	sfx_.erase(it);
	SPDLOG_DEBUG("unloaded sfx {}", name);
	return true;
}

auto app::play_sfx(const std::string &name, const float volume) -> result<> {
	if(sfx_muted_) {
		SPDLOG_DEBUG("sfx is muted, not playing sfx {}", name);
		return true;
	}

	const auto it = sfx_.find(name);
	if(it == sfx_.end()) {
		return error(std::format("can't play sfx with name {}, is not loaded", name));
	}

	const auto &sfx = it->second;
	SetSoundVolume(sfx, volume * sfx_volume_);
	PlaySound(sfx);

	return true;
}

// =============================================================================
// Audio Management - Music
// =============================================================================

auto app::play_music(const std::string &path, const float volume) -> result<> {
	if(current_music_path_ == path && music_playing_) {
		SPDLOG_DEBUG("already playing music {}", path);
		return true;
	}

	if(std::ifstream const font_file(path); !font_file.is_open()) {
		return error(std::format("can not load music file: {}", path));
	}

	if(music_playing_) {
		if(const auto err = stop_music().unwrap(); err) {
			return error("failed to stop previous music", *err);
		}
	}

	background_music_ = LoadMusicStream(path.c_str());
	if(!IsMusicValid(background_music_)) {
		return error(std::format("music stream not valid from path: {}", path));
	}

	background_music_.looping = true;
	PlayMusicStream(background_music_);
	SetMusicVolume(background_music_, music_muted_ ? 0.0F : volume * music_volume_);
	music_playing_ = true;
	current_music_path_ = path;

	SPDLOG_DEBUG("playing music from {}", path);
	return true;
}

auto app::stop_music() -> result<> {
	if(!music_playing_) {
		return error("previous music is not playing");
	}

	StopMusicStream(background_music_);
	UnloadMusicStream(background_music_);
	music_playing_ = false;
	background_music_ = Music{};

	SPDLOG_DEBUG("stopped music");
	return true;
}

// =============================================================================
// Sprite Management
// =============================================================================

auto app::load_sprite_sheet(const std::string &name, const std::string &path) -> result<> {
	if(sprite_sheets_.contains(name)) {
		return error(std::format("sprite sheet with name {} is already loaded", name));
	}

	sprite_sheet sheet;
	if(const auto err = sheet.init(path).unwrap(); err) {
		return error(std::format("failed to load sprite sheet from path: {}", path), *err);
	}

	sprite_sheets_.emplace(name, std::move(sheet));
	SPDLOG_DEBUG("loaded sprite sheet {} from {}", name, path);
	return true;
}

auto app::unload_sprite_sheet(const std::string &name) -> result<> {
	const auto it = sprite_sheets_.find(name);
	if(it == sprite_sheets_.end()) {
		return error(std::format("can't unload sprite sheet with name {}, is not loaded", name));
	}

	if(const auto err = it->second.end().unwrap(); err) {
		return error(std::format("failed to unload sprite sheet with name: {}", name), *err);
	}

	sprite_sheets_.erase(it);
	SPDLOG_DEBUG("unloaded sprite sheet {}", name);
	return true;
}

auto app::draw_sprite(const std::string &sprite_sheet,
					  const std::string &frame,
					  const Vector2 &position,
					  const float &scale,
					  const Color &tint) -> result<> {
	const auto it = sprite_sheets_.find(sprite_sheet);
	if(it == sprite_sheets_.end()) {
		return error(std::format("can't draw sprite, sprite sheet: {}, is not loaded", sprite_sheet));
	}

	const auto &sheet = it->second;
	if(const auto err = sheet.draw(frame, position, scale, tint).unwrap(); err) {
		return error(std::format("failed to draw frame {} from sprite sheet {}", frame, sprite_sheet), *err);
	}

	return true;
}

auto app::get_sprite_size(const std::string &sprite_sheet, const std::string &frame) const -> result<size> {
	const auto it = sprite_sheets_.find(sprite_sheet);
	if(it == sprite_sheets_.end()) {
		return error(std::format("can't get sprite size, sprite sheet: {}, is not loaded", sprite_sheet));
	}

	return it->second.frame_size(frame);
}

auto app::get_sprite_pivot(const std::string &sprite_sheet, const std::string &frame) const -> result<Vector2> {
	const auto it = sprite_sheets_.find(sprite_sheet);
	if(it == sprite_sheets_.end()) {
		return error(std::format("can't get sprite pivot, sprite sheet: {}, is not loaded", sprite_sheet));
	}

	return it->second.frame_pivot(frame);
}

auto app::cleanup_sprite_sheets() -> result<> {
	SPDLOG_INFO("ending sprite sheets");
	for(auto &[name, sheet]: sprite_sheets_) {
		if(const auto err = sheet.end().unwrap(); err) {
			return error(std::format("failed to end sprite sheet with name: {}", name), *err);
		}
		SPDLOG_DEBUG("ended sprite sheet with name: {}", name);
	}
	sprite_sheets_.clear();
	return true;
}

// =============================================================================
// Rendering System
// =============================================================================

auto app::screen_size_changed(const size screen_size) -> result<> {
	screen_size_ = screen_size;

	scale_factor_ = screen_size_.height / design_resolution_.height;
	drawing_resolution_.height = design_resolution_.height;
	drawing_resolution_.width = static_cast<float>(static_cast<int>(screen_size_.width / scale_factor_));

	SPDLOG_DEBUG("display resized, design resolution ({},{}) real resolution ({}x{}), drawing resolution ({}x{}), "
				 "scale factor {}",
				 design_resolution_.width,
				 design_resolution_.height,
				 screen_size_.width,
				 screen_size_.height,
				 drawing_resolution_.width,
				 drawing_resolution_.height,
				 scale_factor_);

	if(const auto err = recreate_render_textures().unwrap(); err) {
		return error("failed to recreate render textures", *err);
	}

	if(const auto err = layout_all_scenes().unwrap(); err) {
		return error("failed to layout scenes", *err);
	}

	update_mouse_scale();

	return true;
}

auto app::cleanup_render_textures() const -> void {
	if(render_texture_.id != 0) {
		UnloadRenderTexture(render_texture_);
	}
	if(shader_texture_.id != 0) {
		UnloadRenderTexture(shader_texture_);
	}
}

auto app::recreate_render_textures() -> result<> {
	cleanup_render_textures();

	render_texture_ =
		LoadRenderTexture(static_cast<int>(drawing_resolution_.width), static_cast<int>(drawing_resolution_.height));
	if(render_texture_.id == 0) {
		return error("failed to create render texture on screen size change");
	}
	SetTextureFilter(render_texture_.texture, TEXTURE_FILTER_POINT);

	shader_texture_ =
		LoadRenderTexture(static_cast<int>(drawing_resolution_.width), static_cast<int>(drawing_resolution_.height));
	if(shader_texture_.id == 0) {
		return error("failed to create shader render texture on screen size change");
	}
	SetTextureFilter(shader_texture_.texture, TEXTURE_FILTER_POINT);

	return true;
}

auto app::update_mouse_scale() const -> void {
	SetMouseScale(1 / scale_factor_, 1 / scale_factor_);
}

auto app::render_scenes_to_texture() const -> result<> {
	BeginTextureMode(render_texture_);
	ClearBackground(clear_color_);

	if(const auto err = draw_all_scenes().unwrap(); err) {
		return error("failed to draw scenes", *err);
	}

	if(const auto err = draw_transition_overlay().unwrap(); err) {
		return error("failed to draw transition overlay", *err);
	}

	EndTextureMode();
	return true;
}

auto app::apply_crt_shader() const -> result<> {
	BeginTextureMode(shader_texture_);
	ClearBackground(BLANK);

	configure_crt_shader();

	BeginShaderMode(crt_shader_);
	DrawTexturePro(render_texture_.texture,
				   {.x = 0.0F,
					.y = 0.0F,
					.width = static_cast<float>(render_texture_.texture.width),
					.height = static_cast<float>(-render_texture_.texture.height)},
				   {.x = 0.0F, .y = 0.0F, .width = drawing_resolution_.width, .height = drawing_resolution_.height},
				   {.x = 0.0F, .y = 0.0F},
				   0.0F,
				   WHITE);
	EndShaderMode();

	if(crt_enabled_) {
		const auto size = crt_texture_.get_size();
		const auto origin = Rectangle(0, 0, size.width, size.height);
		const auto destination = Rectangle(0, 0, drawing_resolution_.width, drawing_resolution_.height);

		if(const auto err = crt_texture_.draw(origin, destination, WHITE, 0.0F, {.x = 0.0F, .y = 0.0F}).unwrap(); err) {
			return error("failed to draw crt overlay texture", *err);
		}
	}

	EndTextureMode();
	return true;
}

auto app::draw_final_output() const -> result<> {
	BeginDrawing();
	ClearBackground(BLACK);

	DrawTexturePro(shader_texture_.texture,
				   {.x = 0.0F,
					.y = 0.0F,
					.width = static_cast<float>(shader_texture_.texture.width),
					.height = static_cast<float>(-shader_texture_.texture.height)},
				   {.x = 0.0F, .y = 0.0F, .width = screen_size_.width, .height = screen_size_.height},
				   {.x = 0.0F, .y = 0.0F},
				   0.0F,
				   WHITE);

	EndDrawing();
	return true;
}

// =============================================================================
// CRT Effect
// =============================================================================

auto app::init_crt_resources() -> result<> {
	if(const auto err = crt_texture_.init(crt_path).unwrap(); err) {
		return error("failed to load crt overlay texture", *err);
	}

	crt_shader_ = LoadShader(crt_shader_vs, crt_shader_fs);
	if(crt_shader_.id == 0) {
		return error("failed to load CRT shader");
	}
	crt_shader_loaded_ = true;

	return true;
}

auto app::cleanup_crt_resources() -> result<> {
	if(const auto err = crt_texture_.end().unwrap(); err) {
		return error("failed to unload crt overlay texture", *err);
	}
	return true;
}

auto app::configure_crt_shader() const -> void {
	const auto swl = GetShaderLocation(crt_shader_, "screen_width");
	SetShaderValue(crt_shader_, swl, &drawing_resolution_.width, SHADER_UNIFORM_FLOAT);

	const auto shl = GetShaderLocation(crt_shader_, "screen_height");
	SetShaderValue(crt_shader_, shl, &drawing_resolution_.height, SHADER_UNIFORM_FLOAT);

	const auto sh_cb = GetShaderLocation(crt_shader_, "color_bleed");
	SetShaderValue(crt_shader_, sh_cb, &color_bleed_, SHADER_UNIFORM_INT);

	const auto sh_sl = GetShaderLocation(crt_shader_, "scan_lines");
	SetShaderValue(crt_shader_, sh_sl, &scan_lines_, SHADER_UNIFORM_INT);
}

// =============================================================================
// Settings
// =============================================================================

auto app::load_settings() -> result<> {
	music_volume_ = settings_.get("music.volume", music_volume_);
	music_muted_ = settings_.get("music.muted", music_muted_);
	sfx_volume_ = settings_.get("sfx.volume", sfx_volume_);
	sfx_muted_ = settings_.get("sfx.muted", sfx_muted_);
	crt_enabled_ = settings_.get("video.crt_enabled", crt_enabled_);
	scan_lines_ = settings_.get("video.scan_lines", scan_lines_);
	color_bleed_ = settings_.get("video.color_bleed", color_bleed_);
#ifndef __EMSCRIPTEN__
	full_screen_ = settings_.get("video.fullscreen", full_screen_);
#endif
	return true;
}

auto app::save_settings() -> result<> {
	return persist_settings();
}

auto app::persist_settings() -> result<> {
	settings_.set("music.volume", music_volume_);
	settings_.set("music.muted", music_muted_);
	settings_.set("sfx.volume", sfx_volume_);
	settings_.set("sfx.muted", sfx_muted_);
	settings_.set("video.crt_enabled", crt_enabled_);
	settings_.set("video.scan_lines", scan_lines_);
	settings_.set("video.color_bleed", color_bleed_);
#ifndef __EMSCRIPTEN__
	settings_.set("video.fullscreen", full_screen_);
#endif

	if(const auto err = settings_.save().unwrap(); err) {
		return error("error saving settings", *err);
	}
	return true;
}

// =============================================================================
// Window Management
// =============================================================================

auto app::init_window() const -> result<> {
#ifdef PLATFORM_DESKTOP
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#endif

	InitWindow(1920, 1080, title_.c_str());
	SetExitKey(KEY_NULL);
	SetTargetFPS(60);

#ifdef PLATFORM_DESKTOP
	const auto icon = LoadImage("resources/icon/icon.png");
	if(icon.width == 0 || icon.height == 0) {
		return error("failed to load window icon");
	}
	SetWindowIcon(icon);
	UnloadImage(icon);
#endif

	return true;
}

auto app::set_fullscreen(const bool fullscreen) -> void {
	if(const auto current_state = is_fullscreen(); current_state != fullscreen) {
		// Need to toggle to reach desired state
		toggle_fullscreen();
	}
}
auto app::is_fullscreen() -> bool {
#ifdef __EMSCRIPTEN__
	// On Emscripten, check native fullscreen state
	full_screen_ = IsWindowFullscreen();
#elif defined(__linux__) || defined(__APPLE__)
	// On Linux and macOS, check maximized state as "fullscreen"
	full_screen_ = IsWindowMaximized();
#else
	// On Windows, check borderless windowed mode
	full_screen_ = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
#endif
	return full_screen_;
}

auto app::toggle_fullscreen() -> bool {
#ifdef __EMSCRIPTEN__
	ToggleFullscreen();
	full_screen_ = IsWindowFullscreen();
#elif defined(__linux__) || defined(__APPLE__)
	// On Linux and macOS, toggle maximized state
	if(IsWindowMaximized()) {
		RestoreWindow();
	} else {
		MaximizeWindow();
	}
	full_screen_ = IsWindowMaximized();
#else
	// Windows uses borderless windowed mode
	ToggleBorderlessWindowed();
	full_screen_ = IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
#endif
	return full_screen_;
}

auto app::handle_escape_key() -> result<> {
	if(!IsKeyReleased(KEY_ESCAPE)) {
		return true;
	}

	std::shared_ptr<scene_info> info;
	if(const auto err = find_scene_info(options_scene_).unwrap(info); err) {
		return error("can not find options scene", *err);
	}

	if(info->scene_ptr->is_visible()) {
		if(const auto hide_err = on_options_closed().unwrap(); hide_err) {
			return error("failed to hide options scene", *hide_err);
		}
	} else {
		if(const auto show_err = on_options_click().unwrap(); show_err) {
			return error("failed to show options scene", *show_err);
		}
	}

	return true;
}

// =============================================================================
// Input Management - Controller
// =============================================================================

auto app::is_controller_button_pressed(const int button) const -> bool {
	return IsGamepadButtonPressed(default_controller_, button);
}

auto app::is_controller_button_down(const int button) const -> bool {
	return IsGamepadButtonDown(default_controller_, button);
}

auto app::is_direction_pressed(const direction check) const -> bool {
	auto pressed = false;
	switch(check) {
	case direction::left:
		pressed = is_controller_button_pressed(GAMEPAD_BUTTON_LEFT_FACE_LEFT);
		break;
	case direction::right:
		pressed = is_controller_button_pressed(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
		break;
	case direction::up:
		pressed = is_controller_button_pressed(GAMEPAD_BUTTON_LEFT_FACE_UP);
		break;
	case direction::down:
		pressed = is_controller_button_pressed(GAMEPAD_BUTTON_LEFT_FACE_DOWN);
		break;
	}

	if(!pressed) {
		const auto trigger_1_x = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_LEFT_X);
		const auto trigger_1_y = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_LEFT_Y);
		const auto trigger_2_x = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_RIGHT_X);
		const auto trigger_2_y = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_RIGHT_Y);

		bool is_active = false;

		switch(check) {
		case direction::left:
			is_active = trigger_1_x < -controller_axis_dead_zone || trigger_2_x < -controller_axis_dead_zone;
			break;
		case direction::right:
			is_active = trigger_1_x > controller_axis_dead_zone || trigger_2_x > controller_axis_dead_zone;
			break;
		case direction::up:
			is_active = trigger_1_y < -controller_axis_dead_zone || trigger_2_y < -controller_axis_dead_zone;
			break;
		case direction::down:
			is_active = trigger_1_y > controller_axis_dead_zone || trigger_2_y > controller_axis_dead_zone;
			break;
		}
		pressed = is_active && !direction_was_active_.at(check);
	}

	return pressed;
}

auto app::is_direction_down(const direction check) const -> bool {
	auto down = false;
	switch(check) {
	case direction::left:
		down = is_controller_button_down(GAMEPAD_BUTTON_LEFT_FACE_LEFT);
		break;
	case direction::right:
		down = is_controller_button_down(GAMEPAD_BUTTON_LEFT_FACE_RIGHT);
		break;
	case direction::up:
		down = is_controller_button_down(GAMEPAD_BUTTON_LEFT_FACE_UP);
		break;
	case direction::down:
		down = is_controller_button_down(GAMEPAD_BUTTON_LEFT_FACE_DOWN);
		break;
	}

	if(!down) {
		const auto trigger_1_x = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_LEFT_X);
		const auto trigger_1_y = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_LEFT_Y);
		const auto trigger_2_x = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_RIGHT_X);
		const auto trigger_2_y = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_RIGHT_Y);

		switch(check) {
		case direction::left:
			down = trigger_1_x < -controller_axis_dead_zone || trigger_2_x < -controller_axis_dead_zone;
			break;
		case direction::right:
			down = trigger_1_x > controller_axis_dead_zone || trigger_2_x > controller_axis_dead_zone;
			break;
		case direction::up:
			down = trigger_1_y < -controller_axis_dead_zone || trigger_2_y < -controller_axis_dead_zone;
			break;
		case direction::down:
			down = trigger_1_y > controller_axis_dead_zone || trigger_2_y > controller_axis_dead_zone;
			break;
		}
	}

	return down;
}

// =============================================================================
// Input Management - Mode Detection
// =============================================================================

auto app::configure_gui_for_input_mode() const -> void {
	if(in_controller_mode_) {
		HideCursor();
		GuiLock();
	} else {
		GuiUnlock();
		ShowCursor();
	}
}

auto app::update_controller_mode(const float delta_time) -> void {
	const auto was_no_controller = default_controller_ == -1;
	default_controller_ = -1;

	for(int i = 0; i < 4; ++i) {
		if(!IsGamepadAvailable(i)) {
			continue;
		}

#ifdef __EMSCRIPTEN__
		const char *name = GetGamepadName(i);
		if(name != nullptr) {
			std::string name_str(name);

			bool is_validated = validated_controllers_.contains(name_str);

			if(!is_validated) {
				bool button_pressed = false;
				for(int button = 0; button <= GAMEPAD_BUTTON_RIGHT_THUMB && !button_pressed; button++) {
					if(IsGamepadButtonPressed(i, button)) {
						button_pressed = true;
					}
				}

				if(button_pressed) {
					validated_controllers_.insert(name_str);
					is_validated = true;
					SPDLOG_DEBUG("validated controller: {}", name_str);
				}
			}

			if(!is_validated) {
				continue;
			}
		}
#endif

		default_controller_ = i;
		break;
	}

	if(default_controller_ != -1 && was_no_controller) {
		SPDLOG_INFO("using controller: {}", GetGamepadName(default_controller_));
		SPDLOG_INFO("controller has {} axis", GetGamepadAxisCount(default_controller_));
	}

	if(default_controller_ == -1 || !IsGamepadAvailable(default_controller_)) {
		in_controller_mode_ = false;
		mouse_inactive_time_ = 0.0F;
		controller_inactive_time_ = 0.0F;
		if(!was_no_controller) {
			SPDLOG_INFO("controller disconnected");
		}
		return;
	}

	if(is_gamepad_input_detected()) {
		controller_inactive_time_ = 0.0F;
		// Only switch to controller mode if grace period expired or already in controller mode
		if(in_controller_mode_ || mouse_inactive_time_ > controller_mode_grace_period) {
			in_controller_mode_ = true;
		}
	}

	if(is_mouse_keyboard_active()) {
		mouse_inactive_time_ = 0.0F;
		// Only switch to mouse mode if grace period expired or already in mouse mode
		if(!in_controller_mode_ || controller_inactive_time_ > controller_mode_grace_period) {
			in_controller_mode_ = false;
		}
	}

	// Update inactive timers
	mouse_inactive_time_ += delta_time;
	controller_inactive_time_ += delta_time;

	// Switch modes after grace period of inactivity from current mode
	if(in_controller_mode_ && controller_inactive_time_ > controller_mode_grace_period) {
		// If controller hasn't been used for a while, allow mouse to take over
		if(mouse_inactive_time_ < controller_inactive_time_) {
			in_controller_mode_ = false;
		}
	} else if(!in_controller_mode_ && mouse_inactive_time_ > controller_mode_grace_period) {
		// If mouse hasn't been used for a while, allow controller to take over
		if(controller_inactive_time_ < mouse_inactive_time_) {
			in_controller_mode_ = true;
		}
	}
}

auto app::is_gamepad_input_detected() const -> bool {
	if(!IsGamepadAvailable(default_controller_)) {
		return false;
	}

	for(int button = 0; button <= GAMEPAD_BUTTON_RIGHT_THUMB; button++) {
		if(IsGamepadButtonPressed(default_controller_, button)) {
			return true;
		}
	}

	constexpr float axis_threshold = 0.3F;
	for(int axis = 0; axis <= GAMEPAD_AXIS_RIGHT_Y; axis++) {
		if(std::abs(GetGamepadAxisMovement(default_controller_, axis)) > axis_threshold) {
			return true;
		}
	}

	return false;
}

auto app::is_mouse_keyboard_active() -> bool {
	constexpr auto delta_threshold = 2.0F;
	const auto [mouse_delta_x, mouse_delta_y] = GetMouseDelta();

	return (std::abs(mouse_delta_x) > delta_threshold) || (std::abs(mouse_delta_y) > delta_threshold)
		   || IsMouseButtonDown(0) || IsMouseButtonDown(1) || IsMouseButtonDown(2) || GetKeyPressed() != 0;
}

auto app::reset_direction_states() -> void {
	if(!is_in_controller_mode()) {
		return;
	}
	const auto trigger_1_x = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_LEFT_X);
	const auto trigger_1_y = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_LEFT_Y);
	const auto trigger_2_x = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_RIGHT_X);
	const auto trigger_2_y = GetGamepadAxisMovement(default_controller_, GAMEPAD_AXIS_RIGHT_Y);

	direction_was_active_[direction::left] =
		trigger_1_x < -controller_axis_dead_zone || trigger_2_x < -controller_axis_dead_zone;
	direction_was_active_[direction::right] =
		trigger_1_x > controller_axis_dead_zone || trigger_2_x > controller_axis_dead_zone;
	direction_was_active_[direction::up] =
		trigger_1_y < -controller_axis_dead_zone || trigger_2_y < -controller_axis_dead_zone;
	direction_was_active_[direction::down] =
		trigger_1_y > controller_axis_dead_zone || trigger_2_y > controller_axis_dead_zone;
}

// =============================================================================
// Logging
// =============================================================================

auto app::setup_log() -> result<> {
#ifdef NDEBUG
	spdlog::set_level(spdlog::level::err);
	SetTraceLogLevel(LOG_ERROR);
#else
	spdlog::set_level(spdlog::level::debug);
	SetTraceLogLevel(LOG_DEBUG);
#endif

	spdlog::set_pattern(empty_format);
	print_banner();
	spdlog::set_pattern(color_line_format);
	SetTraceLogCallback(log_callback);

	SPDLOG_DEBUG("Application: \"{}\", Team: \"{}\", Title: \"{}\"", name_, team_, title_);
	return true;
}

auto app::print_banner() const -> void {
	const auto version_str = std::format("{}.{}.{}.{}", version_.major, version_.minor, version_.patch, version_.build);
	SPDLOG_INFO(std::vformat(banner_, std::make_format_args(version_str)));
}

void app::log_callback(const int log_level, const char *text, va_list args) {
	constexpr std::size_t initial_size = 1024;
	thread_local std::vector<char> buffer(initial_size);

	va_list args_copy{}; // NOLINT(cppcoreguidelines-pro-type-vararg)
	va_copy(args_copy, args);
	int const needed = std::vsnprintf(buffer.data(), buffer.size(), text, args_copy);
	va_end(args_copy);

	if(needed < 0) {
		SPDLOG_ERROR("[raylib] log formatting error in log callback");
		return;
	}

	if(static_cast<std::size_t>(needed) >= buffer.size()) {
		buffer.resize(static_cast<std::size_t>(needed) + 1);
		std::vsnprintf(buffer.data(), buffer.size(), text, args);
	}

	spdlog::level::level_enum level = spdlog::level::info;
	switch(log_level) {
	case LOG_TRACE:
		level = spdlog::level::trace;
		break;
	case LOG_DEBUG:
		level = spdlog::level::debug;
		break;
	case LOG_INFO:
		level = spdlog::level::info;
		break;
	case LOG_WARNING:
		level = spdlog::level::warn;
		break;
	case LOG_ERROR:
		level = spdlog::level::err;
		break;
	case LOG_FATAL:
		level = spdlog::level::critical;
		break;
	default:
		break;
	}

	spdlog::log(level, "[raylib] {}", buffer.data());
}

// =============================================================================
// Utility Functions
// =============================================================================

auto app::parse_version(const std::string &path) -> result<version> {
	std::ifstream const file(path);
	if(!file.is_open()) {
		return error(std::format("version file not found: {}", path));
	}

	std::stringstream buffer;
	buffer << file.rdbuf();

	std::error_code error_code;
	jsoncons::json_decoder<jsoncons::json> decoder;
	jsoncons::json_stream_reader reader(buffer, decoder);
	reader.read(error_code);

	if(error_code) {
		return error(std::format("JSON parse error: {}", error_code.message()));
	}

	const auto &parser = decoder.get_result();

	if(!parser.contains("version")
	   || !parser["version"].is_object()) { // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
		return error("failed to parse version JSON: [\"version\"] field missing or not an object");
	}

	const auto &object = parser["version"]; // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
	return version{.major = object.get_value_or<int>("major", 0),
				   .minor = object.get_value_or<int>("minor", 0),
				   .patch = object.get_value_or<int>("patch", 0),
				   .build = object.get_value_or<int>("build", 0)};
}

auto app::open_url(const std::string &url) -> result<> {
#ifdef _WIN32
	if(auto *result = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, 1);
	   reinterpret_cast<intptr_t>(result) <= 32) { // NOLINT(*-pro-type-reinterpret-cast)
		return error("failed to open URL using shell execute");
	}
	return true;
#elif defined(__APPLE__) || defined(__linux__)
#	ifdef __APPLE__
	const std::string open_command = "open";
#	else
	const std::string open_command = "xdg-open";
#	endif
	const auto pid = fork();
	if(pid == 0) {
		std::vector cmd(open_command.begin(), open_command.end());
		cmd.push_back('\0');
		std::vector arg(url.begin(), url.end());
		arg.push_back('\0');
		const std::vector<char *> argv{cmd.data(), arg.data(), nullptr};
		execvp(cmd.data(), argv.data());
		_exit(1);
	}
	if(pid > 0) {
		return true;
	}
	return error("failed to fork process to open URL");
#elif defined(__EMSCRIPTEN__)
	using emscripten::val;

	const auto document = val::global("document");
	auto anchor = document.call<val>("createElement", val("a"));
	anchor.set("href", url);
	anchor.set("target", "_blank");
	anchor.set("rel", "noopener noreferrer");
	const auto body_list = document.call<val>("getElementsByTagName", val("body"));
	const auto body = body_list.call<val>("item", val(0));

	body.call<void>("appendChild", anchor);
	anchor.call<void>("click");
	body.call<void>("removeChild", anchor);

	return true;
#endif
}

// =============================================================================
// Scene Transition System
// =============================================================================

auto app::start_scene_transition(const scene_id from_scene, const scene_id to_scene) -> void {
	transition_.active = true;
	transition_.stage = transition_stage::fade_out;
	transition_.timer = 0.0F;
	transition_.from_scene = from_scene;
	transition_.to_scene = to_scene;

	// Pause scenes to prevent input during transition
	// For reset transitions (from == to), only pause once
	if(const auto err = pause_scene(from_scene).unwrap(); err) {
		SPDLOG_ERROR("failed to pause from_scene {} during transition start", from_scene);
	}
	if(from_scene != to_scene) {
		if(const auto err = pause_scene(to_scene).unwrap(); err) {
			SPDLOG_ERROR("failed to pause to_scene {} during transition start", to_scene);
		}
	}

	SPDLOG_DEBUG("starting scene transition from {} to {}", from_scene, to_scene);
}

auto app::update_scene_transition(const float delta) -> void {
	if(!transition_.active) {
		return;
	}

	transition_.timer += delta;

	const auto is_reset = transition_.from_scene == transition_.to_scene;

	switch(transition_.stage) {
	case transition_stage::fade_out:
		handle_fade_out_stage(is_reset);
		break;

	case transition_stage::wait:
		handle_wait_stage(is_reset);
		break;

	case transition_stage::fade_in:
		handle_fade_in_stage();
		break;

	case transition_stage::none:
	default:
		break;
	}
}

auto app::handle_fade_out_stage(const bool is_reset) -> void {
	if(transition_.timer < fade_out_duration) {
		return;
	}

	// Fade out complete, check if this is a scene reset or scene change
	if(!is_reset) {
		// Scene change: hide the from scene
		if(const auto err = hide_scene(transition_.from_scene).unwrap(); err) {
			SPDLOG_ERROR("failed to hide scene {} during transition", transition_.from_scene);
		}
	}

	transition_.stage = transition_stage::wait;
	transition_.timer = 0.0F;

	if(is_reset) {
		SPDLOG_DEBUG("transition: fade out complete, entering wait stage (scene reset)");
	} else {
		SPDLOG_DEBUG("transition: fade out complete, entering wait stage");
	}
}

auto app::handle_wait_stage(const bool is_reset) -> void {
	if(transition_.timer < wait_duration) {
		return;
	}

	// Wait complete, resume scene(s) then either reset or show
	// Always resume from_scene
	if(const auto err = resume_scene(transition_.from_scene).unwrap(); err) {
		SPDLOG_ERROR("failed to resume from_scene {} before transition action", transition_.from_scene);
	}

	if(is_reset) {
		// Scene reset: reset the same scene
		if(const auto err = reload_scene(transition_.from_scene).unwrap(); err) {
			SPDLOG_ERROR("failed to reset scene {} during transition", transition_.from_scene);
		}
		SPDLOG_DEBUG("transition: wait complete, scene reset, entering fade in stage");
	} else {
		// Scene change: resume to_scene then show it
		if(const auto err = resume_scene(transition_.to_scene).unwrap(); err) {
			SPDLOG_ERROR("failed to resume to_scene {} before transition action", transition_.to_scene);
		}
		if(const auto err = show_scene(transition_.to_scene).unwrap(); err) {
			SPDLOG_ERROR("failed to show scene {} during transition", transition_.to_scene);
		}
		SPDLOG_DEBUG("transition: wait complete, entering fade in stage");
	}

	transition_.stage = transition_stage::fade_in;
	transition_.timer = 0.0F;
}

auto app::handle_fade_in_stage() -> void {
	if(transition_.timer < fade_in_duration) {
		return;
	}

	// Fade in complete, transition finished
	transition_.active = false;
	transition_.stage = transition_stage::none;
	SPDLOG_DEBUG("transition: fade in complete, transition finished");
}

auto app::draw_transition_overlay() const -> result<> {
	if(!transition_.active) {
		return true;
	}

	auto alpha = 0.0F;

	switch(transition_.stage) {
	case transition_stage::fade_out: {
		// Fade from 0.0 to 1.0
		alpha = std::clamp(transition_.timer / fade_out_duration, 0.0F, 1.0F);
		break;
	}

	case transition_stage::wait:
		// Fully opaque during wait
		alpha = 1.0F;
		break;

	case transition_stage::fade_in: {
		// Fade from 1.0 to 0.0
		const auto progress = std::clamp(transition_.timer / fade_in_duration, 0.0F, 1.0F);
		alpha = 1.0F - progress;
		break;
	}

	case transition_stage::none:
	default:
		break;
	}

	// Create overlay color using clear_color with calculated alpha
	const auto overlay = ColorAlpha(clear_color_, alpha);

	// Draw rectangle covering the entire drawing resolution
	const auto [width, height] = drawing_resolution_;
	DrawRectangle(0, 0, static_cast<int>(width), static_cast<int>(height), overlay);

	return true;
}

} // namespace pxe