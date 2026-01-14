// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/component.hpp>
#include <pxe/result.hpp>

#include <algorithm>
#include <cstddef>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <typeinfo>
#include <utility>
#include <vector>

namespace pxe {

class app;

class scene: public component {
public:
	struct child {
		std::shared_ptr<component> comp;
		int layer = 0;
	};
	[[nodiscard]] auto init(app &app) -> result<> override {
		return component::init(app);
	}

	[[nodiscard]] auto end() -> result<> override;

	[[nodiscard]] virtual auto show() -> result<> {
		return true;
	}

	[[nodiscard]] virtual auto hide() -> result<> {
		return true;
	}

	[[nodiscard]] auto update(float delta) -> result<> override;

	[[nodiscard]] auto draw() -> result<> override;

	[[nodiscard]] virtual auto reset() -> result<> {
		return true;
	}

	[[nodiscard]] virtual auto layout(const size /*screen_size*/) -> result<> {
		return true;
	}

	template<typename T, typename... Args>
		requires std::is_base_of_v<component, T>
	[[nodiscard]] auto register_component(Args &&...args) -> result<size_t> {
		auto comp = std::make_shared<T>();
		if(const auto err = comp->init(get_app(), std::forward<Args>(args)...).unwrap(); err) {
			return error(std::format("error initializing component of type: {}", typeid(T).name()), *err);
		}
		auto id = comp->get_id(); // save id before moving
		children_.emplace_back(child{.comp = std::move(comp), .layer = 0});
		SPDLOG_DEBUG("component of type `{}` registered with id {}", typeid(T).name(), id);
		return id;
	}

	[[nodiscard]] auto remove_component(const size_t id) -> result<> {
		const auto it = find_component(id);
		if(it == children_.end()) {
			return error(std::format("no component found with id: {}", id));
		}
		if(const auto err = it->comp->end().unwrap(); err) {
			return error(std::format("error ending component with id: {}", id), *err);
		}
		children_.erase(it);
		SPDLOG_DEBUG("component with id {} removed", id);
		return true;
	}

	template<typename T>
		requires std::is_base_of_v<component, T>
	[[nodiscard]] auto get_component(const size_t id) -> result<std::shared_ptr<T>> {
		const auto it = find_component(id);
		if(it == children_.end()) {
			return error(std::format("no component found with id: {}", id));
		}
		auto comp = std::dynamic_pointer_cast<T>(it->comp);
		if(!comp) {
			return error(std::format("component with id: {} is not of type: {}", id, typeid(T).name()));
		}
		return comp;
	}

private:
	[[nodiscard]] auto find_component(const size_t id) -> std::vector<child>::iterator {
		return std::ranges::find_if(children_, [id](const child &c) -> bool { return c.comp->get_id() == id; });
	}

	std::vector<child> children_;
};
} // namespace pxe