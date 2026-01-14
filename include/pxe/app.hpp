// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/events.hpp>
#include <pxe/render/sprite_sheet.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <raylib.h>

#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>
#include <typeinfo>
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
		: name_{std::move(name)}, team_{std::move(team)}, title_{std::move(title)},
		  design_resolution_(design_resolution), banner_{std::move(banner)} {}
	virtual ~app() = default;

	// Non-copyable
	app(const app &) = delete;
	auto operator=(const app &) -> app & = delete;

	// Non-movable
	app(app &&) noexcept = delete;
	auto operator=(app &&) noexcept -> app & = delete;

	[[nodiscard]] auto run() -> result<>;

	struct version {
		int major{};
		int minor{};
		int patch{};
		int build{};
	};

	[[nodiscard]] auto get_version() const -> const version & {
		return version_;
	}

	[[nodiscard]] auto get_default_font() const -> const Font & {
		return default_font_;
	}

	[[nodiscard]] auto get_default_font_size() const -> int {
		return default_font_size_;
	}

	template<typename Event>
	auto subscribe(std::function<result<>(const Event &)> handler) -> int {
		return event_bus_.subscribe<Event>(std::move(handler));
	}

	template<typename Event, typename T, typename Func>
	auto bind_event(T *instance, Func func) -> int {
		return subscribe<Event>([instance, func](const Event &evt) -> result<> { return (instance->*func)(evt); });
	}

	template<typename Event, typename T, typename Func>
	auto on_event(T *instance, Func func) -> int {
		return subscribe<Event>([instance, func](const Event &) -> result<> { return (instance->*func)(); });
	}

	auto unsubscribe(const int token) -> void {
		event_bus_.unsubscribe(token);
	}

	template<typename Event>
	auto post_event(const Event &event) -> void {
		event_bus_.post(event);
	}

	[[nodiscard]] auto play_sound(const std::string &name, float volume = 1.0F) -> result<>;

	[[nodiscard]] auto load_sprite_sheet(const std::string &name, const std::string &path) -> result<>;
	[[nodiscard]] auto unload_sprite_sheet(const std::string &name) -> result<>;
	[[nodiscard]] auto draw_sprite(const std::string &sprite_sheet,
								   const std::string &frame,
								   const Vector2 &position,
								   const float &scale,
								   const Color &tint = WHITE) -> result<>;

	[[nodiscard]] auto get_sprite_size(const std::string &sprite_sheet, const std::string &frame) const -> result<size>;

	[[nodiscard]] auto get_sprite_pivot(const std::string &sprite_sheet, const std::string &frame) const
		-> result<Vector2>;

	[[nodiscard]] auto play_music(const std::string &path, float volume = 1.0F) -> result<>;
	[[nodiscard]] auto stop_music() -> result<>;

	auto close() -> void;
	auto toggle_fullscreen() -> bool;

protected:
	[[nodiscard]] virtual auto init() -> result<>;
	[[nodiscard]] virtual auto init_scenes() -> result<>;
	[[nodiscard]] virtual auto end() -> result<>;
	[[nodiscard]] virtual auto update() -> result<>;
	[[nodiscard]] virtual auto draw() const -> result<>;

	auto set_clear_color(const Color &color) -> void {
		clear_color_ = color;
	}

	template<typename T>
		requires std::is_base_of_v<scene, T>
	auto register_scene(int layer = 0, const bool visible = true) -> int {
		int id = ++last_scene_id_;

		std::string name;
#if defined(__GNUG__) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
		int status = 0;
		const char *mangled = typeid(T).name();
		using demangle_ptr = std::unique_ptr<char, decltype(&std::free)>;
		demangle_ptr const demangled{abi::__cxa_demangle(mangled, nullptr, nullptr, &status), &std::free};
		name = (status == 0 && demangled) ? demangled.get() : mangled;
#else
		name = typeid(T).name();
#endif
		SPDLOG_DEBUG("registering scene of type `{}` with id {} at layer {}", name, id, layer);
		const auto scene_info_ptr = std::make_shared<scene_info>(
			scene_info{.id = id, .name = name, .scene_ptr = std::make_unique<T>(), .layer = layer, .visible = visible});
		scenes_.push_back(scene_info_ptr);
		sort_scenes();
		return id;
	}

	template<typename T>
		requires std::is_base_of_v<scene, T>
	auto register_scene(const bool visible) -> int {
		return register_scene<T>(0, visible);
	}

	auto unregister_scene(const int id) -> result<> {
		const auto it = std::ranges::find_if(scenes_, [id](const auto &scene) -> bool { return scene->id == id; });
		if(it != scenes_.end()) {
			if((*it)->scene_ptr) {
				if(const auto err = (*it)->scene_ptr->end().unwrap(); err) {
					return error(std::format("error ending scene with id: {} name: {}", id, (*it)->name), *err);
				}
				(*it)->scene_ptr.reset();
			}
			scenes_.erase(it);
			return true;
		}
		return error(std::format("scene with id {} not found", id));
	}

	auto sort_scenes() -> void {
		std::ranges::sort(
			scenes_, [](const auto &scene_a, const auto &scene_b) -> bool { return scene_a->layer < scene_b->layer; });
	}

	[[nodiscard]] auto show_scene(int id, bool show = true) -> result<>;

	[[nodiscard]] auto hide_scene(const int id, const bool hide = true) -> result<> {
		return show_scene(id, !hide);
	}

	[[nodiscard]] auto reset_scene(int id) -> result<>;

	[[nodiscard]] auto
	set_default_font(const std::string &path, int size = 0, int texture_filter = TEXTURE_FILTER_POINT) -> result<>;

	auto set_default_font_size(const int size) -> void {
		default_font_size_ = size;
	}

	[[nodiscard]] auto load_sound(const std::string &name, const std::string &path) -> result<>;
	[[nodiscard]] auto unload_sound(const std::string &name) -> result<>;

private:
	std::string name_;
	std::string team_;

	Font default_font_{};
	int default_font_size_{12};

	bool custom_default_font_{false};
	int last_scene_id_{0};

	std::string title_{"Engine App"};
	size screen_size_{};
	static constexpr auto version_file_path = "resources/version/version.json";
	version version_{};

	Color clear_color_ = WHITE;

	[[nodiscard]] auto setup_log() -> result<>;
	[[nodiscard]] static auto parse_version(const std::string &path) -> result<version>;
	static void log_callback(int log_level, const char *text, va_list args);

	struct scene_info {
		int id{};
		std::string name;
		std::unique_ptr<scene> scene_ptr{nullptr};
		int layer{};
		bool visible{};
	};

	std::vector<std::shared_ptr<scene_info>> scenes_;

	auto find_scene_info(const int id) -> result<std::shared_ptr<scene_info>> {
		for(auto &scene_info: scenes_) {
			if(scene_info->id == id) {
				return scene_info;
			}
		}
		return error(std::format("scene with id {} not found", id));
	}

	auto set_default_font(const Font &font, int size, int texture_filter = TEXTURE_FILTER_POINT) -> void;

	event_bus event_bus_;

	auto init_sound() -> result<>;
	auto end_sound() -> result<>;
	bool sound_initialized_{false};

	std::unordered_map<std::string, Sound> sounds_;

	Music background_music_{};
	bool music_playing_{false};
	std::string current_music_path_;

	auto update_music_stream() const -> void;

	[[nodiscard]] auto screen_size_changed(size screen_size) -> result<>;
	size design_resolution_;
	size drawing_resolution_{};
	float scale_factor_{1.0F};

	RenderTexture2D render_texture_{};

	std::string banner_ = "Engine Application v{}";

	std::unordered_map<std::string, sprite_sheet> sprite_sheets_;

	auto on_version_click() -> result<>;
	int version_click_{0};
	static auto open_url(const std::string &url) -> result<>;

	bool full_screen_{false};
	bool should_exit_{false};
};

} // namespace pxe
