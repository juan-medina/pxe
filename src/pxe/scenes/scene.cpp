#include <pxe/components/component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <algorithm>
#include <format>
#include <memory>

namespace pxe {

auto scene::end() -> result<> {
	for(auto &[comp, layer, type_name]: children_) {
		if(const auto err = comp->end().unwrap(); err) {
			return error(std::format("error ending component with id: {} name: {}", comp->get_id(), type_name), *err);
		}
	}
	return component::end();
}
auto scene::update(const float delta) -> result<> {
	for(auto &[comp, layer, type_name]: children_) {
		if(const auto err = comp->update(delta).unwrap(); err) {
			return error(std::format("error updating component with id: {} name: {}", comp->get_id(), type_name), *err);
		}
	}
	return component::update(delta);
}

auto scene::draw() -> result<> {
	std::ranges::sort(children_, [](const child &a, const child &b) -> bool { return a.layer < b.layer; });
	for(auto &[comp, layer, type_name]: children_) {
		if(const auto err = comp->draw().unwrap(); err) {
			return error(std::format("error drawing component with id: {} name: {}", comp->get_id(), type_name), *err);
		}
	}
	return component::draw();
}
auto scene::pause() -> result<> {
	set_enabled(false);
	paused_components_.clear();
	for(auto &[comp, layer, type_name]: children_) {
		const auto id = comp->get_id();
		const auto was_enabled = comp->is_enabled();
		auto pause_result = paused_component{.id = id, .enabled = was_enabled};
		paused_components_.emplace_back(pause_result);
		comp->set_enabled(false);
	}
	return true;
}

auto scene::resume() -> result<> {
	set_enabled(true);
	for(const auto &paused_comp: paused_components_) {
		std::shared_ptr<component> comp;
		if(const auto err = get_component<component>(paused_comp.id).unwrap(comp); err) {
			return error(std::format("failed to get component with id: {}", paused_comp.id), *err);
		}
		comp->set_enabled(paused_comp.enabled);
	}
	paused_components_.clear();
	return true;
}

} // namespace pxe