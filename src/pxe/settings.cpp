// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/result.hpp>
#include <pxe/settings.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <jsoncons/basic_json.hpp>
#include <jsoncons/json_decoder.hpp>
#include <jsoncons/json_reader.hpp>
#include <regex>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <variant>

#ifdef __EMSCRIPTEN__
#	include <emscripten/val.h>
#	include <stdexcept>
#else
#	include <platform_folders.h>
#endif

namespace pxe {

auto settings::init(const std::string &team, const std::string &application) -> result<> {
	team_ = team;
	application_ = application;

#ifdef __EMSCRIPTEN__
	SPDLOG_DEBUG("using localStorage for settings");
	return true;
#else
	if(const auto err = get_settings_path().unwrap(file_path_); err) {
		return error("error getting settings path.", *err);
	}

	if(const auto err = load().unwrap(); err) {
		return error("error loading settings.", *err);
	}

	return true;
#endif
}

auto settings::end() const -> result<> {
#ifndef __EMSCRIPTEN__
	if(const auto err = save().unwrap(); err) {
		return error("error saving settings on end.", *err);
	}
#endif
	return true;
}

auto settings::simplify_name(const std::string &name) -> std::string {
	const std::string result = std::regex_replace(name, std::regex{R"([\s])"}, "_");
	return std::regex_replace(result, std::regex{R"([^\w])"}, "");
}

#ifdef __EMSCRIPTEN__
auto settings::get_storage_key(const std::string &key) const -> std::string {
	return std::format("{}.{}.{}", simplify_name(team_), simplify_name(application_), key);
}

template<typename T>
auto settings::save_to_local_storage(const std::string &key, const T &value) const -> void {
	using emscripten::val;
	const auto storage = val::global("localStorage");
	const auto storage_key = get_storage_key(key);

	if constexpr(std::is_same_v<T, int>) {
		storage.call<void>("setItem", val(storage_key), val(std::to_string(value)));
	} else if constexpr(std::is_same_v<T, float>) {
		storage.call<void>("setItem", val(storage_key), val(std::to_string(value)));
	} else if constexpr(std::is_same_v<T, bool>) {
		storage.call<void>("setItem", val(storage_key), val(value ? "true" : "false"));
	} else if constexpr(std::is_same_v<T, std::string>) {
		storage.call<void>("setItem", val(storage_key), val(value));
	}
}

template<typename T>
auto settings::load_from_local_storage(const std::string &key, const T &default_value) const -> T {
	using emscripten::val;
	const auto storage = val::global("localStorage");
	const auto storage_key = get_storage_key(key);

	const auto item = storage.call<val>("getItem", storage_key);
	if(item.isNull() || item.isUndefined()) {
		return default_value;
	}

	const auto str_value = item.as<std::string>();
	if(str_value.empty()) {
		return default_value;
	}

	if constexpr(std::is_same_v<T, int>) {
		try {
			return std::stoi(str_value);
		} catch(...) {
			return default_value;
		}
	} else if constexpr(std::is_same_v<T, float>) {
		try {
			return std::stof(str_value);
		} catch(...) {
			return default_value;
		}
	} else if constexpr(std::is_same_v<T, bool>) {
		return str_value == "true";
	} else if constexpr(std::is_same_v<T, std::string>) {
		return str_value;
	}

	return default_value;
}

// Explicit template instantiations
template auto settings::save_to_local_storage<int>(const std::string &, const int &) const -> void;
template auto settings::save_to_local_storage<float>(const std::string &, const float &) const -> void;
template auto settings::save_to_local_storage<bool>(const std::string &, const bool &) const -> void;
template auto settings::save_to_local_storage<std::string>(const std::string &, const std::string &) const -> void;

template auto settings::load_from_local_storage<int>(const std::string &, const int &) const -> int;
template auto settings::load_from_local_storage<float>(const std::string &, const float &) const -> float;
template auto settings::load_from_local_storage<bool>(const std::string &, const bool &) const -> bool;
template auto settings::load_from_local_storage<std::string>(const std::string &, const std::string &) const
	-> std::string;
#endif

auto settings::save() const -> result<> {
#ifdef __EMSCRIPTEN__
	SPDLOG_DEBUG("settings saved to localStorage");
	return true;
#else
	jsoncons::ojson json = jsoncons::ojson::object();

	for(const auto &[fst, snd]: settings_) {
		const auto &key = fst;
		const auto &val = snd;
		if(std::holds_alternative<int>(val)) {
			json[key] = std::get<int>(val); // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
		} else if(std::holds_alternative<float>(val)) {
			json[key] = std::get<float>(val); // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
		} else if(std::holds_alternative<bool>(val)) {
			json[key] = std::get<bool>(val); // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
		} else if(std::holds_alternative<std::string>(val)) {
			json[key] = std::get<std::string>(val); // NOLINT(*-pro-bounds-avoid-unchecked-container-access)
		}
	}

	std::ofstream file(file_path_);
	if(!file.is_open()) {
		return error(std::format("can't open settings file for writing: {}", file_path_.string()));
	}

	file << json << '\n';
	if(file.fail()) {
		return error(std::format("failed writing settings to file: {}", file_path_.string()));
	}

	file.close();

	SPDLOG_DEBUG("saved settings to {}", file_path_.string());
	return true;
#endif
}

auto settings::load() -> result<> {
#ifdef __EMSCRIPTEN__
	return true;
#else
	std::ifstream file(file_path_);
	if(!file.is_open()) {
		return error(std::format("can't open settings file for reading: {}", file_path_.string()));
	}

	file.seekg(0, std::ios::end);
	if(file.tellg() == 0) {
		SPDLOG_DEBUG("settings file is empty, starting with default settings");
		return true;
	}
	file.seekg(0, std::ios::beg);

	std::error_code error_code;
	jsoncons::json_decoder<jsoncons::json> decoder;
	jsoncons::json_stream_reader reader(file, decoder);
	reader.read(error_code);

	if(error_code) {
		return error(std::format("failed parsing settings file: {}", error_code.message()));
	}

	const auto json = decoder.get_result();

	for(const auto &member: json.object_range()) {
		const auto &key = member.key();
		const auto &val = member.value();

		if(val.is_int64()) {
			settings_[key] = val.as<int>();
		} else if(val.is_double()) {
			settings_[key] = val.as<float>();
		} else if(val.is_bool()) {
			settings_[key] = val.as<bool>();
		} else if(val.is_string()) {
			settings_[key] = val.as<std::string>();
		} else {
			return error(std::format("unsupported settings value type for key: {}", key));
		}
	}

	SPDLOG_DEBUG("loaded settings from {}", file_path_.string());
	return true;
#endif
}

auto settings::get_settings_path() const -> result<std::filesystem::path> {
	namespace fs = std::filesystem;

	auto simple_team = simplify_name(team_);
	auto simple_application = simplify_name(application_);

	if(simple_team.empty() || simple_application.empty()) {
		return error("error getting the path for settings file.");
	}

#ifdef __EMSCRIPTEN__
	auto home = ".";
#else
	auto home = sago::getConfigHome();
#endif

	fs::path const home_folder{home};
	if(!fs::exists(home_folder)) {
		return error("can't get game settings directory.");
	}

	fs::path const team_path{simple_team};
	fs::path const team_full_path = home_folder / team_path;
	if(auto err = exist_or_create_directory(team_full_path).unwrap(); err) {
		return error("can't get game settings directory.", *err);
	}

	fs::path const application_path{simple_application};
	fs::path const application_full_path = team_full_path / application_path;
	if(auto err = exist_or_create_directory(application_full_path).unwrap(); err) {
		return error("can't get game settings directory.", *err);
	}

	fs::path const settings_file_full_path = application_full_path / "settings.json";
	if(auto err = exist_or_create_file(settings_file_full_path).unwrap(); err) {
		return error("can't find or create settings file.", *err);
	}

	return settings_file_full_path;
}

auto settings::exist_or_create_directory(const std::filesystem::path &path) -> result<> {
	namespace fs = std::filesystem;
	if(fs::exists(path)) {
		SPDLOG_DEBUG("directory already exist: {}", path.string());
	} else {
		SPDLOG_DEBUG("creating directory: {}", path.string());
		std::error_code error_code;
		fs::create_directories(path, error_code);
		if(error_code) {
			return error(std::format("can't create directory {}: {}", path.string(), error_code.message()));
		}
		if(!fs::exists(path)) {
			return error(std::format("directory doesn't exist after creation: {}", path.string()));
		}
	}
	return true;
}

auto settings::exist_or_create_file(const std::filesystem::path &path) -> result<> {
	namespace fs = std::filesystem;
	if(fs::exists(path)) {
		SPDLOG_DEBUG("file already exist: {}", path.string());
	} else {
		SPDLOG_DEBUG("creating empty file: {}", path.string());
		std::fstream file;
		file.open(path, std::ios::out);
		if(file.fail()) {
			return error(std::format("can't create file: {}", path.string()));
		}
		file.close();
		if(file.fail()) {
			return error(std::format("can't close file: {}", path.string()));
		}
		if(!fs::exists(path)) {
			return error(std::format("file doesn't exist after creation: {}", path.string()));
		}
	}
	return true;
}

} // namespace pxe
