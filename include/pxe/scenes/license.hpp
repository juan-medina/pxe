// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/button.hpp>
#include <pxe/components/component.hpp>
#include <pxe/result.hpp>
#include <pxe/scenes/scene.hpp>

#include <cstddef>

namespace pxe {
class app;
struct size;

class license: public scene {
public:
	license() = default;
	~license() override = default;

	// Copyable
	license(const license &) = default;
	auto operator=(const license &) -> license & = default;

	// Movable
	license(license &&) noexcept = default;
	auto operator=(license &&) noexcept -> license & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;

	auto layout(size screen_size) -> result<> override;

	struct accepted {};

private:
	static constexpr auto license_path = "resources/license/license.txt";
	size_t scroll_text_{0};
	size_t accept_button_{0};
	int button_click_{0};

	auto on_button_click(const button::click &evt) -> result<>;
};

} // namespace pxe
