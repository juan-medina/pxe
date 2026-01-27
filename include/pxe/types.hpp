// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <string>

#if defined(__GNUG__) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
#	include <cxxabi.h>
#	include <memory>
#	include <type_traits>
#endif

namespace pxe {

template<typename T>
static auto get_type_name() -> std::string {
#if defined(__GNUG__) && !defined(__EMSCRIPTEN__) && !defined(__APPLE__)
	int status = 0;
	const char *mangled = typeid(T).name();
	using demangle_ptr = std::unique_ptr<char, decltype(&std::free)>;
	demangle_ptr const demangled{abi::__cxa_demangle(mangled, nullptr, nullptr, &status), &std::free};
	return (status == 0 && demangled) ? demangled.get() : mangled;
#else
	return typeid(T).name();
#endif
}

} // namespace pxe