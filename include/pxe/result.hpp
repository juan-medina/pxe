// SPDX-FileCopyrightText: 2026 Juan Medina
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <variant>
#include <vector>

namespace pxe {

class error {
public:
	explicit error(const std::string &message, const std::source_location &location = std::source_location::current())
		: causes_{cause{.message = message, .location = location}} {}

	error(const std::string &message,
		  const error &other,
		  const std::source_location &location = std::source_location::current())
		: causes_{cause{.message = message, .location = location}} {
		causes_.insert(causes_.end(), other.causes_.begin(), other.causes_.end());
	}

	error(const std::string &message,
		  const std::optional<error> &other,
		  const std::source_location &location = std::source_location::current())
		: causes_{cause{.message = message, .location = location}} {
		if(other) {
			causes_.insert(causes_.end(), other->causes_.begin(), other->causes_.end());
		}
	}

	[[nodiscard]] auto get_message() const noexcept -> const std::string & {
		return causes_.front().message;
	}

	[[nodiscard]] auto get_location() const noexcept -> const std::source_location & {
		return causes_.front().location;
	}

	[[nodiscard]] auto to_string() const -> std::string {
		std::string out;
		for(size_t i = 0; i < causes_.size(); ++i) {
			const auto &[message, location] = causes_.at(i);
			if(i == 0) {
				out += format_message_with_location(message, location);
			} else {
				out += "\n  caused by: " + format_message_with_location(message, location);
			}
		}
		return out;
	}

private:
	struct cause {
		std::string message;
		std::source_location location;
	};

	static auto format_message_with_location(const std::string &msg, const std::source_location &loc) -> std::string {
		return std::format("{} [{}:{} {}]", msg, loc.file_name(), loc.line(), loc.function_name());
	}

	std::vector<cause> causes_;
};

template<class Value = bool, class Error = error>
class result: public std::variant<Value, Error> {
public:
	// NOLINTNEXTLINE(google-explicit-constructor)
	result(const Value &value): std::variant<Value, Error>(value) {}

	// NOLINTNEXTLINE(google-explicit-constructor)
	result(const Error &error): std::variant<Value, Error>(error) {}

	[[maybe_unused]] auto has_error() const noexcept {
		return std::holds_alternative<Error>(*this);
	}

	[[maybe_unused]] auto has_value() const noexcept {
		return std::holds_alternative<Value>(*this);
	}

	[[maybe_unused]] [[nodiscard]] auto get_error() const noexcept {
		return std::get<Error>(*this);
	}

	[[maybe_unused]] [[nodiscard]] auto get_value() const noexcept {
		return std::get<Value>(*this);
	}

	auto unwrap(Value &value) const noexcept -> std::optional<Error> {
		if(has_error()) {
			return std::get<Error>(*this);
		}
		value = std::get<Value>(*this);
		return std::nullopt;
	}

	auto unwrap(const Value &value) const noexcept -> std::optional<Error> {
		if(has_error()) {
			return std::get<Error>(*this);
		}
		value = std::get<Value>(*this);
		return std::nullopt;
	}

	auto unwrap() const noexcept -> std::optional<Error> {
		if(has_error()) {
			return std::get<Error>(*this);
		}
		return std::nullopt;
	}
};

} // namespace pxe