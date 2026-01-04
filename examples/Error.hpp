// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <string>

enum class ErrorTag : unsigned char
{
  DATABASE,   ///< Errors related to database operations
  VALIDATION, ///< Errors related to input validation
};

/**
 * @brief Converts an ErrorTag enum value to its string representation.
 * @param tag The error tag to convert.
 * @return A C-string representing the tag name.
 */
[[nodiscard]]
auto inline toString(ErrorTag tag) noexcept -> const char*
{
  switch (tag)
  {
  case ErrorTag::DATABASE:
    return "DATABASE";
  case ErrorTag::VALIDATION:
    return "VALIDATION";
  default:
    return "UNKNOWN";
  }
}

class Error
{
  std::string _message;
  ErrorTag _tag;
  unsigned _code;

public:
  /**
   * @brief Constructs an Error object.
   * @param tag The error tag.
   * @param message A descriptive error message.
   * @param code Optional error code for handling strategy (defaults to
   * unsigned(-1)).
   */
  Error(ErrorTag tag, std::string message, unsigned code = unsigned(-1))
      : _message(std::move(message)), _tag(tag), _code(code)
  {
  }

  /// @brief Gets the error tag.
  [[nodiscard]]
  auto tag() const noexcept -> ErrorTag
  {
    return _tag;
  }

  /// @brief Gets reference to the error message.
  [[nodiscard]]
  auto message() const noexcept -> const std::string&
  {
    return _message;
  }

  /// @brief Gets the error code.
  [[nodiscard]]
  auto code() const noexcept -> unsigned
  {
    return _code;
  }
};
