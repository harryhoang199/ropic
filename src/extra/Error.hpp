// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <string>

namespace ropic
{ /**
   * @enum ErrorTag
   * @brief Classifies errors based on the most general and prominent criteria.
   *
   * ErrorTag values must not be added arbitrarily; they require discussion and consensus.
   * Classification criteria include:
   * - **Boundary Errors**: Each connection to an external system corresponds to a distinct
   *   tag (e.g., DATABASE, NETWORK, FILE_SYSTEM, EXTERNAL_API).
   * - **By Module**: Errors thrown by a specific big module or subsystem (e.g., AUTHENTICATION,
   *   PARSER, RENDERER).
   * - **By Frequency**: Based on how often the error occurs (e.g., VALIDATION for common
   *   input validation errors).
   * - **By Severity**: Critical vs non-critical errors (e.g., FATAL, WARNING).
   * - **By Recovery Strategy**: Errors that share the same recovery approach (e.g., RETRYABLE,
   *   NON_RETRYABLE).
   */
  enum class ErrorTag : unsigned
  {
    DATABASE,   ///< Errors related to database operations
    VALIDATION, ///< Errors related to input validation
  };

  /**
   * @brief Converts an ErrorTag enum value to its string representation.
   * @param tag The error tag to convert.
   * @return A C-string representing the tag name.
   */
  [[nodiscard]] auto toString(ErrorTag tag) noexcept -> const char *
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

  // ==========================================
  // ERROR CLASS DEFINITION
  // ==========================================

  /**
   * @class Error
   * @brief Represents an error with tag, message, and optional error code.
   *
   * The Error class encapsulates error information with two key components:
   *
   * **tag**: Classifies errors based on the most general and prominent criteria.
   * See ErrorTag documentation for classification guidelines.
   *
   * **code**: Used to distinguish errors based on the **handling strategy**.
   * - This is only necessary when the caller invokes a function prone to various specific
   *   errors and intends to handle them using different approaches/strategies.
   * - In the majority of cases, this is not required → defaults to `-1` (unsigned max).
   * - Prefer using an `enum` instead of hardcoded values with careful documentation:
   *   @code
   *   enum class Strategy : unsigned {
   *       STRATEGY_1,
   *       STRATEGY_2
   *   };
   *   @endcode
   *
   * **message**: A human-readable description of the error.
   */
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
     * @param code Optional error code for handling strategy (defaults to unsigned(-1)).
     */
    Error(ErrorTag tag, std::string message, unsigned code = unsigned(-1))
        : _tag(tag), _message(std::move(message)), _code(code)
    {
    }

    /// @brief Gets the error tag.
    [[nodiscard]] auto tag() const noexcept -> ErrorTag { return _tag; }

    /// @brief Gets reference to the error message.
    [[nodiscard]] auto message() const noexcept -> const std::string & { return _message; }

    /// @brief Gets the error code.
    [[nodiscard]] auto code() const noexcept -> unsigned { return _code; }
  };
}