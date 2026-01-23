// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <pxe/app.hpp>

#ifndef __EMSCRIPTEN__
#	include <boxer/boxer.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#	include <minwindef.h>
#endif

namespace pxe {

template<typename App_Type>
	requires std::is_base_of_v<app, App_Type>
auto run_app() -> int {
	try {
		App_Type application;
		if(const auto error = application.run().unwrap(); error) {
			SPDLOG_ERROR("{}", error->to_string());
#ifndef __EMSCRIPTEN__
			boxer::show(error->get_message().c_str(), "Error!", boxer::Style::Error);
#endif
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	} catch(const std::exception &e) {
		std::fputs("unhandled exception in main: ", stderr);
		std::fputs(e.what(), stderr);
		std::fputc('\n', stderr);
	} catch(...) {
		std::fputs("unhandled non-standard exception in main\n", stderr);
	}
	return EXIT_FAILURE;
}

} // namespace pxe

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define PXE_MAIN(AppClass) \
	auto main(int, char *[]) -> int { \
		return pxe::run_app<AppClass>(); \
	} \
\
	IF_WIN32(auto WINAPI WinMain([[maybe_unused]] HINSTANCE instance, \
								 [[maybe_unused]] HINSTANCE prev_instance, \
								 [[maybe_unused]] LPSTR cmd_line, \
								 [[maybe_unused]] int show) \
				 ->int { return pxe::run_app<AppClass>(); })

#ifdef _WIN32
#	define IF_WIN32(...) __VA_ARGS__ // NOLINT(cppcoreguidelines-macro-usage)
#else
#	define IF_WIN32(...) // NOLINT(cppcoreguidelines-macro-usage)
#endif