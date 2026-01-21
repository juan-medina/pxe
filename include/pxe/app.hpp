// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/events.hpp>
#include <pxe/render/sprite_sheet.hpp>
#include <pxe/render/texture.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>
#include <pxe/settings.hpp>

#include <raylib.h>

#include <algorithm>
#include <cstdarg>
#include <functional>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(__GNUG__) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
#	include <cxxabi.h>
#endif

namespace pxe {

class sprite_sheet;

class app {
public:
	explicit app(
		std::string name, std::string team, std::string title, std::string banner, const size design_resolution)
		: name_{std::move(name)}, team_{std::move(team)}, title_{std::move(title)}, banner_{std::move(banner)},
		  design_resolution_(design_resolution) {}
	virtual ~app() = default;

	app(const app &) = delete;
	auto operator=(const app &) -> app & = delete;
	app(app &&) noexcept = delete;
	auto operator=(app &&) noexcept -> app & = delete;

	[[nodiscard]] auto run() -> result<>;
	auto close() -> void;

	struct version {
		int major{};
		int minor{};
		int patch{};
		int build{};
	};

	[[nodiscard]] auto get_version() const -> const version & {
		return version_;
	}

	// Font Management
	[[nodiscard]] auto get_default_font() const -> const Font & {
		return default_font_;
	}

	[[nodiscard]] auto get_default_font_size() const -> int {
		return default_font_size_;
	}

	auto set_default_font_size(const int size) -> void {
		default_font_size_ = size;
	}

	// Event System
	template<typename Event>
	auto subscribe(std::function<result<>(const Event &)> handler) -> int {
		return event_bus_.subscribe<Event>(std::move(handler));
	}

	template<typename Event, typename T, typename Func>
	[[nodiscard]] auto bind_event(T *instance, Func func) -> int {
		return subscribe<Event>([instance, func](const Event &evt) -> result<> { return (instance->*func)(evt); });
	}

	template<typename Event, typename T, typename Func>
	[[nodiscard]] auto on_event(T *instance, Func func) -> int {
		return subscribe<Event>([instance, func](const Event &) -> result<> { return (instance->*func)(); });
	}

	auto unsubscribe(const int token) -> void {
		event_bus_.unsubscribe(token);
	}

	template<typename Event>
	auto post_event(const Event &event) -> void {
		event_bus_.post(event);
	}

	// Audio Management - Music
	[[nodiscard]] auto play_music(const std::string &path, float volume = 1.0F) -> result<>;
	[[nodiscard]] auto stop_music() -> result<>;

	auto set_music_volume(const float volume) -> void {
		music_volume_ = std::clamp(volume, 0.0F, 1.0F);
		if(music_playing_) {
			SetMusicVolume(background_music_, music_volume_);
		}
	}

	[[nodiscard]] auto get_music_volume() const -> float {
		return music_volume_;
	}

	auto set_music_muted(const bool muted) -> void {
		music_muted_ = muted;
		if(music_playing_) {
			SetMusicVolume(background_music_, music_muted_ ? 0.0F : music_volume_);
		}
	}

	[[nodiscard]] auto is_music_muted() const -> bool {
		return music_muted_;
	}

	// Audio Management - SFX
	[[nodiscard]] auto play_sfx(const std::string &name, float volume = 1.0F) -> result<>;

	auto set_sfx_volume(const float volume) -> void {
		sfx_volume_ = std::clamp(volume, 0.0F, 1.0F);
	}

	[[nodiscard]] auto get_sfx_volume() const -> float {
		return sfx_volume_;
	}

	auto set_sfx_muted(const bool muted) -> void {
		sfx_muted_ = muted;
	}

	[[nodiscard]] auto is_sfx_muted() const -> bool {
		return sfx_muted_;
	}

	// Sprite Management
	[[nodiscard]] auto load_sprite_sheet(const std::string &name, const std::string &path) -> result<>;
	[[nodiscard]] auto unload_sprite_sheet(const std::string &name) -> result<>;
	[[nodiscard]] auto draw_sprite(const std::string &sprite_sheet,
								   const std::string &frame,
								   const Vector2 &position,
								   const float &scale = 1.0F,
								   const Color &tint = WHITE) -> result<>;
	[[nodiscard]] auto get_sprite_size(const std::string &sprite_sheet, const std::string &frame) const -> result<size>;
	[[nodiscard]] auto get_sprite_pivot(const std::string &sprite_sheet, const std::string &frame) const
		-> result<Vector2>;

	// Display Settings
	auto toggle_fullscreen() -> bool;

	[[nodiscard]] auto is_crt_enabled() const -> bool {
		return crt_enabled_;
	}

	[[nodiscard]] auto is_color_bleed_enabled() const -> bool {
		return color_bleed_ == 1;
	}

	[[nodiscard]] auto is_scan_lines_enabled() const -> bool {
		return scan_lines_ == 1;
	}

	auto set_crt_enabled(const bool enabled) -> void {
		crt_enabled_ = enabled;
	}

	auto set_color_bleed_enabled(const bool enabled) -> void {
		color_bleed_ = enabled ? 1 : 0;
	}

	auto set_scan_lines_enabled(const bool enabled) -> void {
		scan_lines_ = enabled ? 1 : 0;
	}

	// Settings Persistence
	template<typename T>
	auto get_setting(const std::string &key, T default_value) -> T {
		return settings_.get(key, default_value);
	}

	template<typename T>
	auto set_setting(const std::string &key, const T &value) -> void {
		settings_.set(key, value);
	}

	[[nodiscard]] auto save_settings() -> result<>;

	// Input Mode
	[[nodiscard]] auto is_in_controller_mode() const -> bool {
		return in_controller_mode_;
	}

	struct back_to_menu {};

	// Input Management
	[[nodiscard]] auto is_controller_button_pressed(int button) const -> bool;
	[[nodiscard]] auto is_direction_pressed(direction check) const -> bool;

	[[nodiscard]] auto is_controller_button_down(int button) const -> bool;
	[[nodiscard]] auto is_direction_down(direction check) const -> bool;

protected:
	[[nodiscard]] virtual auto init() -> result<>;
	[[nodiscard]] virtual auto init_scenes() -> result<>;
	[[nodiscard]] virtual auto end() -> result<>;
	[[nodiscard]] virtual auto update() -> result<>;
	[[nodiscard]] virtual auto draw() const -> result<>;

	auto set_clear_color(const Color &color) -> void {
		clear_color_ = color;
	}

	// Scene Registration
	template<typename T>
		requires std::is_base_of_v<scene, T>
	auto register_scene(int layer = 0, const bool visible = true) -> scene_id {
		auto id = ++last_scene_id_;
		const auto name = get_scene_type_name<T>();

		SPDLOG_DEBUG("registering scene of type `{}` with id {} at layer {}", name, id, layer);

		const auto scene_info_ptr = std::make_shared<scene_info>(
			scene_info{.id = id, .name = name, .scene_ptr = std::make_unique<T>(), .layer = layer});
		scenes_.push_back(scene_info_ptr);

		scene_info_ptr->scene_ptr->set_visible(visible);
		sort_scenes();

		return id;
	}

	template<typename T>
		requires std::is_base_of_v<scene, T>
	auto register_scene(const bool visible) -> scene_id {
		return register_scene<T>(0, visible);
	}

	[[nodiscard]] auto unregister_scene(scene_id id) -> result<>;
	[[nodiscard]] auto show_scene(scene_id id, bool show = true) -> result<>;

	[[nodiscard]] auto hide_scene(const scene_id id, const bool hide = true) -> result<> {
		return show_scene(id, !hide);
	}

	[[nodiscard]] auto reset_scene(scene_id id) -> result<>;
	auto set_main_scene(const scene_id &scene) -> void {
		main_scene_ = scene;
	}

	// Resource Loading (Protected)
	[[nodiscard]] auto
	set_default_font(const std::string &path, int size = 0, int texture_filter = TEXTURE_FILTER_POINT) -> result<>;
	[[nodiscard]] auto load_sfx(const std::string &name, const std::string &path) -> result<>;
	[[nodiscard]] auto unload_sfx(const std::string &name) -> result<>;

private:
	// Application Identity
	std::string name_;
	std::string team_;
	std::string title_{"Engine App"};
	std::string banner_{"Engine Application v{}"};
	version version_{};
	static constexpr auto version_file_path = "resources/version/version.json";

	// Font Management
	Font default_font_{};
	int default_font_size_{12};
	bool custom_default_font_{false};

	auto set_default_font(const Font &font, int size, int texture_filter = TEXTURE_FILTER_POINT) -> void;

	// Event System
	event_bus event_bus_;

	// Scene Management
	struct scene_info {
		scene_id id;
		std::string name;
		std::unique_ptr<scene> scene_ptr{nullptr};
		int layer{};
	};

	std::vector<std::shared_ptr<scene_info>> scenes_;
	scene_id last_scene_id_{0};
	scene_id main_scene_{0};
	scene_id license_scene_{0};
	scene_id menu_scene_{0};
	scene_id options_scene_{0};

	[[nodiscard]] auto find_scene_info(scene_id id) -> result<std::shared_ptr<scene_info>>;
	auto sort_scenes() -> void;
	[[nodiscard]] auto init_all_scenes() -> result<>;
	[[nodiscard]] auto end_all_scenes() -> result<>;
	[[nodiscard]] auto update_all_scenes(float delta) const -> result<>;
	[[nodiscard]] auto draw_all_scenes() const -> result<>;
	[[nodiscard]] auto layout_all_scenes() const -> result<>;

	template<typename T>
	static auto get_scene_type_name() -> std::string {
#if defined(__GNUG__) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
		int status = 0;
		const char *mangled = typeid(T).name();
		using demangle_ptr = std::unique_ptr<char, decltype(&std::free)>;
		demangle_ptr const demangled{abi::__cxa_demangle(mangled, nullptr, nullptr, &status), &std::free};
		return (status == 0 && demangled) ? demangled.get() : mangled;
#else
		return typeid(T).name();
#endif
	}

	// Scene Event Handlers
	int version_click_{0};
	int options_click_{0};
	int options_closed_{0};
	int license_accepted_{0};
	int go_to_game_{0};
	int back_to_menu_{0};

	[[nodiscard]] auto on_version_click() const -> result<>;
	[[nodiscard]] auto on_options_click() -> result<>;
	[[nodiscard]] auto on_options_closed() -> result<>;
	[[nodiscard]] auto on_license_accepted() -> result<>;
	[[nodiscard]] auto on_go_to_game() -> result<>;
	[[nodiscard]] auto on_back_to_menu() -> result<>;

	auto register_builtin_scenes() -> void;
	auto subscribe_to_builtin_events() -> void;
	auto unsubscribe_from_builtin_events() -> void;

	// Audio System
	bool audio_initialized_{false};
	std::unordered_map<std::string, Sound> sfx_;
	Music background_music_{};
	bool music_playing_{false};
	std::string current_music_path_;
	float music_volume_{0.5F};
	bool music_muted_{false};
	float sfx_volume_{1.0F};
	bool sfx_muted_{false};

	auto init_audio() -> result<>;
	auto end_audio() -> result<>;
	auto update_music_stream() const -> void;
	auto cleanup_audio_resources() -> result<>;

	// Sprite Management
	std::unordered_map<std::string, sprite_sheet> sprite_sheets_;

	[[nodiscard]] auto cleanup_sprite_sheets() -> result<>;

	// Rendering System
	Color clear_color_{WHITE};
	size screen_size_{};
	size design_resolution_;
	size drawing_resolution_{};
	float scale_factor_{1.0F};
	RenderTexture2D render_texture_{};
	RenderTexture2D shader_texture_{};

	[[nodiscard]] auto screen_size_changed(size screen_size) -> result<>;
	auto cleanup_render_textures() const -> void;
	[[nodiscard]] auto recreate_render_textures() -> result<>;
	auto update_mouse_scale() const -> void;
	[[nodiscard]] auto render_scenes_to_texture() const -> result<>;
	[[nodiscard]] auto apply_crt_shader() const -> result<>;
	[[nodiscard]] auto draw_final_output() const -> result<>;

	// CRT Effect
	texture crt_texture_;
	static auto constexpr crt_path = "resources/bg/crt.png";
	Shader crt_shader_{};
	bool crt_shader_loaded_{false};
	static auto constexpr crt_shader_vs = "resources/shaders/crt.vs";
	static auto constexpr crt_shader_fs = "resources/shaders/crt.fs";
	bool crt_enabled_{true};
	int scan_lines_{1};
	int color_bleed_{1};

	[[nodiscard]] auto init_crt_resources() -> result<>;
	[[nodiscard]] auto cleanup_crt_resources() -> result<>;
	auto configure_crt_shader() const -> void;

	// Settings
	settings settings_;

	[[nodiscard]] auto load_settings() -> result<>;
	[[nodiscard]] auto persist_settings() -> result<>;

	// Window Management
	bool full_screen_{false};
	bool should_exit_{false};

	[[nodiscard]] auto init_window() const -> result<>;
	[[nodiscard]] auto handle_escape_key() -> result<>;

	// Input Management
	bool in_controller_mode_{false};
	float mouse_inactive_time_{0.0F};
	static constexpr float controller_mode_grace_period = 2.0F;

	auto update_controller_mode(float delta_time) -> void;
	[[nodiscard]] auto is_gamepad_input_detected() const -> bool;
	[[nodiscard]] static auto is_mouse_keyboard_active() -> bool;


#ifdef __EMSCRIPTEN__
	std::unordered_set<std::string> validated_controllers_;
#endif
	auto reset_direction_states() -> void;

	static constexpr auto controller_axis_dead_zone = 0.3F;
	std::unordered_map<direction, bool> direction_was_active_;

	// Main Loop
	[[nodiscard]] auto main_loop() -> result<>;
	auto configure_gui_for_input_mode() const -> void;

	// Logging
	[[nodiscard]] auto setup_log() -> result<>;
	static void log_callback(int log_level, const char *text, va_list args);
	auto print_banner() const -> void;

	// Utility
	[[nodiscard]] static auto parse_version(const std::string &path) -> result<version>;
	static auto open_url(const std::string &url) -> result<>;
	int default_controller_ = 0;

};

} // namespace pxe
