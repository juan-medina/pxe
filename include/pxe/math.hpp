// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

namespace pxe {

class oscillator {
public:
	explicit oscillator(const float amplitude): amplitude_(amplitude) {}

	auto update(float delta) -> void;

	[[nodiscard]] auto get_value() const -> float;

	auto reset() -> void;

private:
	float phase_{0.0F};
	float amplitude_{1.0F};
};

} // namespace pxe