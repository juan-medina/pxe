// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <cstddef>
#include <string>

namespace pxe {

class checkbox: public ui_component {
public:
	[[nodiscard]] auto draw() -> result<> override;

	auto set_title(const std::string &title) -> void;

	[[nodiscard]] auto get_title() const -> std::string {
		return title_;
	}

	auto set_checked(const bool checked) -> void {
		checked_ = checked;
	}

	[[nodiscard]] auto is_checked() const -> bool {
		return checked_;
	}

	struct checkbox_changed {
		size_t id{};
		bool checked{false};
	};

private:
	std::string title_;
	bool checked_{false};
	float check_box_size_{0.0F};
};

} // namespace pxe