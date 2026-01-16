// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/result.hpp>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>

namespace pxe {

class settings {
public:
	[[nodiscard]] auto init(const std::string &team, const std::string &application) -> result<>;
	[[nodiscard]] auto end() const -> result<>;

	template<typename T>
	auto set(const std::string &key, const T &new_value) -> void {
		settings_.insert_or_assign(key, value{new_value});
#ifdef __EMSCRIPTEN__
		save_to_local_storage(key, new_value);
#endif
	}

	template<typename T>
	auto get(const std::string &key, T default_value) -> T {
#ifdef __EMSCRIPTEN__
		return load_from_local_storage(key, default_value);
#else
		auto result = default_value;
		if(const auto it = settings_.find(key); it != settings_.end()) {
			if(std::holds_alternative<T>(it->second)) {
				result = std::get<T>(it->second);
			}
		}
		settings_.insert_or_assign(key, value{result});
		return result;
#endif
	}

	[[nodiscard]] auto save() const -> result<>;

private:
	std::string team_;
	std::string application_;
	using value = std::variant<int, float, bool, std::string>;
	std::unordered_map<std::string, value> settings_;
	std::filesystem::path file_path_;

	auto load() -> result<>;

	[[nodiscard]] auto get_settings_path() const -> result<std::filesystem::path>;
	static auto simplify_name(const std::string &name) -> std::string;
	static auto exist_or_create_directory(const std::filesystem::path &path) -> result<>;
	static auto exist_or_create_file(const std::filesystem::path &path) -> result<>;

#ifdef __EMSCRIPTEN__
	auto get_storage_key(const std::string &key) const -> std::string;

	template<typename T>
	auto save_to_local_storage(const std::string &key, const T &val) const -> void;

	template<typename T>
	auto load_from_local_storage(const std::string &key, const T &default_value) const -> T;
#endif
};

} // namespace pxe
