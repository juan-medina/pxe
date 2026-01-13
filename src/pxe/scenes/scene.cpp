#include <pxe/components/component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <algorithm>
#include <format>

namespace pxe {

auto scene::end() -> result<> {
	for(auto &[comp, layer]: children_) {
		if(const auto err = comp->end().unwrap(); err) {
			return error(std::format("error ending component with id: {}", comp->get_id()), *err);
		}
	}
	return component::end();
}
auto scene::update(const float delta) -> result<> {
	for(auto &[comp, layer]: children_) {
		if(const auto err = comp->update(delta).unwrap(); err) {
			return error(std::format("error updating component with id: {}", comp->get_id()), *err);
		}
	}
	return component::update(delta);
}

auto scene::draw() -> result<> {
	std::ranges::sort(children_, [](const child &a, const child &b) -> bool { return a.layer < b.layer; });
	for(auto &[comp, layer]: children_) {
		if(const auto err = comp->draw().unwrap(); err) {
			return error(std::format("error drawing component with id: {}", comp->get_id()), *err);
		}
	}
	return component::draw();
}

} // namespace pxe