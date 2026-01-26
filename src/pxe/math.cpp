// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#include <pxe/math.hpp>

#include <cmath>
#include <numbers>

namespace pxe {

auto oscillator::update(float delta) -> void {
	static constexpr auto pi = std::numbers::pi_v<float>;
	static constexpr auto pi2 = 2.0F * pi;
	static constexpr float cycles_per_second{2.0F};
	phase_ = std::fmod(phase_ + (delta * cycles_per_second * pi2), pi2);
}

auto oscillator::get_value() const -> float {
	return std::sin(phase_) * amplitude_;
}

auto oscillator::reset() -> void {
	phase_ = 0.0F;
}

} // namespace pxe
