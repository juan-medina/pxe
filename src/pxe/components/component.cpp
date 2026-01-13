// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/components/component.hpp>
#include <pxe/result.hpp>

#include <cstddef>

namespace pxe {

size_t component::next_id = 0;

auto component::init(app &app) -> result<> {
	app_ = app;
	return true;
}
auto component::end() -> result<> {
	app_.reset();
	return true;
}

} // namespace pxe
