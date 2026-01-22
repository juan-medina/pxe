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

class about: public scene {
public:
	about() = default;
	~about() override = default;

	// Copyable
	about(const about &) = default;
	auto operator=(const about &) -> about & = default;

	// Movable
	about(about &&) noexcept = default;
	auto operator=(about &&) noexcept -> about & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;

	auto layout(size screen_size) -> result<> override;

	struct back_clicked {};

private:
	static constexpr auto about_path = "resources/about/about.txt";
	size_t scroll_text_{0};
	size_t back_button_{0};
	int button_click_{0};

	auto on_button_click(const button::click &evt) -> result<>;
};

} // namespace pxe
