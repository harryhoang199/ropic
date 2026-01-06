// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cstdint>

namespace ropic::detail
{
/**
 * @enum Unit
 * @brief Unit type representing a successful void-like operation.
 *
 * Use this type as the DATA parameter for Either when the operation
 * does not return a meaningful value, e.g., `Either<Void, Error>`.
 */
enum class Unit : std::uint8_t
{
  OK ///< The single value representing success.
};
} // namespace ropic::detail

namespace ropic
{
/// @brief Type alias for Unit, used as DATA type for void-like Either
/// operations.
using Void = detail::Unit;

/// @brief Convenience constant for returning success from void-like operations.
constexpr Void OK = detail::Unit::OK;

/// @brief Alias for OK constant.
constexpr Void VOID = detail::Unit::OK;
} // namespace ropic