// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/components/ui_component.hpp>
#include <pxe/result.hpp>

#include <string>

namespace pxe {

class window: public ui_component {
public:
	[[nodiscard]] auto draw() -> result<> override;

	auto set_title(const std::string &title) -> void {
		title_ = title;
	}

	struct close {};

private:
	std::string title_;
};

} // namespace pxe
