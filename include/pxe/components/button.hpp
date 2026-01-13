// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>
#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <cstddef>
#include <string>

namespace pxe {
class app;

class button: public ui_component {
public:
	button() = default;
	~button() override = default;

	// Copyable
	button(const button &) = default;
	auto operator=(const button &) -> button & = default;

	// Movable
	button(button &&) noexcept = default;
	auto operator=(button &&) noexcept -> button & = default;

	[[nodiscard]] auto init(app &app) -> result<> override;
	[[nodiscard]] auto end() -> result<> override;

	[[nodiscard]] auto update(float delta) -> result<> override;
	[[nodiscard]] auto draw() -> result<> override;

	auto set_text(const std::string &text) -> void {
		text_ = text;
	}

	[[nodiscard]] auto get_text() const -> const std::string & {
		return text_;
	}

	struct click {
		size_t id{};
	};

private:
	std::string text_{"Button"};
};

} // namespace pxe