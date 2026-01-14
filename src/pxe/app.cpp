// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include "pxe/events.hpp"
#include "pxe/scenes/scene.hpp"
#include <pxe/app.hpp>
#include <pxe/components/component.hpp>
#include <pxe/render/sprite_sheet.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/game_overlay.hpp>

#include "spdlog/fmt/bundled/base.h"

#include <raylib.h>

#include <cstdio>
#include <format>
#include <fstream>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/json_decoder.hpp>
#include <jsoncons/json_reader.hpp>
#include <jsoncons/source.hpp>
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

auto app::init() -> result<> {
	if(const auto err = parse_version(version_file_path).unwrap(version_); err) {
		return error("error parsing the version", *err);
	}

	SPDLOG_DEBUG("parsed version: {}.{}.{}.{}", version_.major, version_.minor, version_.patch, version_.build);

	if(const auto err = setup_log().unwrap(); err) {
		return error("error initializing the application", *err);
	}

	if(const auto err = init_sound().unwrap(); err) {
		return error("audio device could not be initialized", *err);
	}

	version_click_ = on_event<game_overlay::version_click>(this, &app::on_version_click);

	SPDLOG_INFO("init application");

#ifdef PLATFORM_DESKTOP
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#endif
	InitWindow(1920, 1080, title_.c_str());
	SetTargetFPS(60);

#ifdef PLATFORM_DESKTOP
	const auto icon = LoadImage("resources/icon/icon.png");
	if(icon.width == 0 || icon.height == 0) {
		return error("failed to load window icon");
	}
	SetWindowIcon(icon);
	UnloadImage(icon);
#endif
	default_font_ = GetFontDefault();

	// register default scenes
	register_scene<game_overlay>(999);

	return true;
}

auto app::init_scenes() -> result<> {
	// init scenes
	SPDLOG_INFO("init scenes");
	for(auto &info: scenes_) {
		if(const auto err = info->scene_ptr->init(*this).unwrap(); err) {
			return error(std::format("failed to initialize scene with id: {} name: {}", info->id, info->name), *err);
		}
		SPDLOG_DEBUG("initialized scene with id: {} name: {}", info->id, info->name);
	}
	return true;
}

auto app::end() -> result<> {
	unsubscribe(version_click_);

	// end scenes
	SPDLOG_INFO("ending scenes");
	for(auto &info: scenes_) {
		if(info->scene_ptr) {
			if(const auto err = info->scene_ptr->end().unwrap(); err) {
				return error(std::format("error ending scene with id: {} name: {}", info->id, info->name), *err);
			}
			SPDLOG_DEBUG("end scene with id: {} name: {}", info->id, info->name);
			info->scene_ptr.reset();
		}
	}
	scenes_.clear();

	if(custom_default_font_) {
		SPDLOG_DEBUG("unloading custom default font");
		UnloadFont(default_font_);
	}

	if(const auto err = end_sound().unwrap(); err) {
		return error("audio device could not be ended", *err);
	}

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

auto app::run() -> result<> {
	if(const auto err = init().unwrap(); err) {
		return error("error init the application", *err);
	}

	if(const auto err = init_scenes().unwrap(); err) {
		return error("error init scenes", *err);
	}

	while(!should_exit_) {
		should_exit_ = should_exit_ || WindowShouldClose();
		SetMouseScale(1 / scale_factor_, 1 / scale_factor_);
		if(const auto err = update().unwrap(); err) {
			return error("error updating the application", *err);
		}
		if(const auto err = draw().unwrap(); err) {
			return error("error drawing the application", *err);
		}
	}

	if(const auto err = end().unwrap(); err) {
		return error("error ending the application", *err);
	}

	SPDLOG_INFO("application ended");
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

	// update scenes
	for(auto &info: scenes_) {
		if(!info->visible) {
			continue;
		}
		if(const auto err = info->scene_ptr->update(GetFrameTime()).unwrap(); err) {
			return error(std::format("failed to update scene with id: {} name: {}", info->id, info->name), *err);
		}
	}

	// dispatch events
	if(const auto err = event_bus_.dispatch().unwrap(); err) {
		return error("error dispatching events", *err);
	}

	update_music_stream();

	return true;
}

auto app::setup_log() -> result<> {
#ifdef NDEBUG
	spdlog::set_level(spdlog::level::err);
	SetTraceLogLevel(LOG_ERROR);
#else
	spdlog::set_level(spdlog::level::debug);
	SetTraceLogLevel(LOG_DEBUG);
#endif
	spdlog::set_pattern(empty_format);
	const auto version_str = std::format("{}.{}.{}.{}", version_.major, version_.minor, version_.patch, version_.build);
	SPDLOG_INFO(std::vformat(banner_, std::make_format_args(version_str)));

	spdlog::set_pattern(color_line_format);
	SetTraceLogCallback(log_callback);

	SPDLOG_DEBUG("Application: \"{}\", Team: \"{}\", Title: \"{}\"", name_, team_, title_);
	return true;
}

void app::log_callback(const int log_level, const char *text, va_list args) {
	constexpr std::size_t initial_size = 1024;

	// One buffer per thread, reused across calls
	thread_local std::vector<char> buffer(initial_size);

	va_list args_copy{};	  // NOLINT(*-pro-type-vararg)
	va_copy(args_copy, args); // NOLINT(*-pro-bounds-array-to-pointer-decay)
	int const needed =
		std::vsnprintf(buffer.data(), buffer.size(), text, args_copy); // NOLINT(*-pro-bounds-array-to-pointer-decay)
	va_end(args_copy);												   // NOLINT(*-pro-bounds-array-to-pointer-decay)

	if(needed < 0) {
		SPDLOG_ERROR("[raylib] log formatting error in log callback");
		return;
	}

	if(static_cast<std::size_t>(needed) >= buffer.size()) {
		// Grow once (or very rarely)
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

auto app::draw() const -> result<> {
	BeginTextureMode(render_texture_);
	ClearBackground(clear_color_);

	// draw scenes
	for(const auto &info: scenes_) {
		if(!info->visible) {
			continue;
		}
		if(const auto err = info->scene_ptr->draw().unwrap(); err) {
			return error(std::format("failed to draw scene with id: {} name:", info->id, info->name), *err);
		}
	}

	EndTextureMode();
	BeginDrawing();
	ClearBackground(BLACK);
	DrawTexturePro(render_texture_.texture,
				   {.x = 0.0F,
					.y = 0.0F,
					.width = static_cast<float>(render_texture_.texture.width),
					.height = static_cast<float>(-render_texture_.texture.height)},
				   {.x = 0.0F, .y = 0.0F, .width = screen_size_.width, .height = screen_size_.height},
				   {.x = 0.0F, .y = 0.0F},
				   0.0F,
				   WHITE);
	EndDrawing();
	return true;
}

auto app::show_scene(const int id, const bool show) -> result<> {
	std::shared_ptr<scene_info> info;
	if(const auto err = find_scene_info(id).unwrap(info); err) {
		return error(std::format("scene with id {} not found", id));
	}
	info->visible = show;
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

auto app::reset_scene(const int id) -> result<> {
	std::shared_ptr<scene_info> info;
	if(const auto err = find_scene_info(id).unwrap(info); err) {
		return error(std::format("scene with id {} not found", id));
	}
	if(const auto reset_err = info->scene_ptr->reset().unwrap(); reset_err) {
		return error(std::format("failed to re-enable scene with id: {} name: {}", id, info->name), *reset_err);
	}
	SPDLOG_DEBUG("reset scene with id: {} name: {}", id, info->name);
	// layout on reset
	if(const auto layout_err = info->scene_ptr->layout(drawing_resolution_).unwrap(); layout_err) {
		return error(std::format("failed to layout scene with id: {} name: {}", id, info->name), *layout_err);
	}

	return true;
}

auto app::set_default_font(const std::string &path, const int size, const int texture_filter) -> result<> {
	auto font_size = size;
	if(std::ifstream const font_file(path); !font_file.is_open()) {
		return error(std::format("can not load  font file: {}", path));
	}

	if(custom_default_font_) {
		SPDLOG_DEBUG("unloading previous custom default font");
		UnloadFont(default_font_);
		custom_default_font_ = false;
	}

	const auto font = LoadFontEx(path.c_str(), font_size, nullptr, 0);
	if(font_size == 0) {
		font_size = font.baseSize;
	}

	set_default_font(font, font_size, texture_filter);

	custom_default_font_ = true;
	SPDLOG_DEBUG("set default font to {}", path);
	return true;
}
auto app::load_sound(const std::string &name, const std::string &path) -> result<> {
	if(std::ifstream const font_file(path); !font_file.is_open()) {
		return error(std::format("can not load sound file: {}", path));
	}

	if(sounds_.contains(name)) {
		return error(std::format("sound with name {} is already loaded", name));
	}

	Sound const sound = LoadSound(path.c_str());

	if(!IsSoundValid(sound)) {
		return error(std::format("sound not valid from path: {}", path));
	}

	sounds_.emplace(name, sound);
	SPDLOG_DEBUG("loaded sound {} from {}", name, path);

	return true;
}

auto app::unload_sound(const std::string &name) -> result<> {
	const auto it = sounds_.find(name);
	if(it == sounds_.end()) {
		return error(std::format("can't unload sound with name {}, is not loaded", name));
	}

	UnloadSound(it->second);
	sounds_.erase(it);
	SPDLOG_DEBUG("unloaded sound {}", name);
	return true;
}

auto app::play_music(const std::string &path, const float volume /* - 1.0F*/) -> result<> {
	if(current_music_path_ == path && music_playing_) {
		SPDLOG_TRACE("already playing music {}", path);
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
	SetMusicVolume(background_music_, volume);
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

void app::close() {
	should_exit_ = true;
}

auto app::toggle_fullscreen() -> bool {
	full_screen_ = !full_screen_;
#ifdef __EMSCRIPTEN__
	ToggleFullscreen();
	full_screen_ = IsWindowFullscreen();
#elif defined(__linux__)
	if(IsWindowMaximized()) {
		RestoreWindow();
	} else {
		MaximizeWindow();
	}
	full_screen_ = IsWindowMaximized();
#else
	ToggleBorderlessWindowed();
#endif

	return full_screen_;
}

auto app::play_sound(const std::string &name, const float volume /*= 1.0F*/) -> result<> {
	const auto it = sounds_.find(name);
	if(it == sounds_.end()) {
		return error(std::format("can't play sound with name {}, is not loaded", name));
	}
	const auto &sound = it->second;
	SetSoundPitch(sound, volume);
	PlaySound(sound);

	return true;
}
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
	const auto &sheet = it->second;

	return sheet.frame_size(frame);
}

auto app::get_sprite_pivot(const std::string &sprite_sheet, const std::string &frame) const -> result<Vector2> {
	const auto it = sprite_sheets_.find(sprite_sheet);
	if(it == sprite_sheets_.end()) {
		return error(std::format("can't get sprite pivot, sprite sheet: {}, is not loaded", sprite_sheet));
	}
	const auto &sheet = it->second;

	return sheet.frame_pivot(frame);
}

auto app::set_default_font(const Font &font, const int size, const int texture_filter) -> void {
	default_font_ = font;
	default_font_size_ = size;
	SetTextureFilter(font.texture, texture_filter);
	GuiSetFont(default_font_);
	GuiSetStyle(DEFAULT, TEXT_SIZE, size);
}

auto app::init_sound() -> result<> {
	InitAudioDevice();
	if(IsAudioDeviceReady()) {
		sound_initialized_ = true;
		SPDLOG_INFO("audio device initialized");
		return true;
	}
	return error("failed to initialize audio device");
}

auto app::end_sound() -> result<> {
	for(const auto &[name, sound]: sounds_) {
		if(IsSoundPlaying(sound)) {
			StopSound(sound);
			SPDLOG_DEBUG("stopped playing sound {}", name);
		}
	}

	for(const auto &[name, sound]: sounds_) {
		UnloadSound(sound);
		SPDLOG_DEBUG("unloaded sound {}", name);
	}
	sounds_.clear();

	if(music_playing_) {
		if(const auto err = stop_music().unwrap(); err) {
			return error("failed to stop music during app end", *err);
		}
	}

	if(sound_initialized_) {
		CloseAudioDevice();
		sound_initialized_ = false;
		SPDLOG_INFO("audio device closed");
		return true;
	}
	SPDLOG_WARN("audio device was not initialized");
	return true;
}
auto app::update_music_stream() const -> void {
	if(music_playing_) {
		UpdateMusicStream(background_music_);
	}
}
auto app::screen_size_changed(const size screen_size) -> result<> {
	screen_size_ = screen_size;

	// scale is base on y-axis, x-axis is adjusted to keep aspect ratio
	//  technically we want to draw more horizontally if the screen is wider
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

	if(render_texture_.id != 0) {
		UnloadRenderTexture(render_texture_);
	}
	render_texture_ =
		LoadRenderTexture(static_cast<int>(drawing_resolution_.width), static_cast<int>(drawing_resolution_.height));
	if(render_texture_.id == 0) {
		return error("failed to create render texture on screen size change");
	}

	SetTextureFilter(render_texture_.texture, TEXTURE_FILTER_POINT);

	// screen size changed, tell scenes to layout
	for(const auto &scene_info: scenes_) {
		if(const auto err = scene_info->scene_ptr->layout(drawing_resolution_).unwrap(); err) {
			return error(std::format("failed to layout scene with id: {} name: {}", scene_info->id, scene_info->name),
						 *err);
		}
	}

	return true;
}

auto app::on_version_click() -> result<> {
	return open_url("https://github.com/juan-medina/energy-swap/releases");
}

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

	// NOLINTNEXTLINE(*-pro-bounds-avoid-unchecked-container-access)
	if(!parser.contains("version") || !parser["version"].is_object()) {
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

} // namespace pxe