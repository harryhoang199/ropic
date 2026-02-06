// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <coroutine>

#include "either_impl.hpp"

#include "ropic/void.hpp"

namespace ropic::detail
{
/// @brief Reference type for awaited EitherImpl: lvalue ref or rvalue ref.
template <typename OTHER, typename ERROR, bool IS_EITHER_LREF>
using AwaitableEither = std::conditional_t<
    IS_EITHER_LREF,
    EitherImpl<OTHER, ERROR>&,
    EitherImpl<OTHER, ERROR>&&>;

/**
 * @brief Awaiter for interoperability with non-Either coroutines (Task,
 * Generator, etc.).
 *
 * Used when co_await-ing an Either from outside the Either coroutine
 * ecosystem. Always ready (never suspends). Returns the EitherImpl object
 * itself without unwrapping.
 */
template <typename VALUE, typename ERROR, bool IS_EITHER_LREF>
class InteropAwaiter
{
  /// @brief The awaited EitherImpl (reference or rvalue based on
  /// IS_EITHER_LREF).
  AwaitableEither<VALUE, ERROR, IS_EITHER_LREF> _awaitableEither;

public:
  /// @brief Constructs awaiter from rvalue EitherImpl (takes ownership).
  explicit InteropAwaiter(EitherImpl<VALUE, ERROR>&& awaitableEither)
      noexcept(std::is_nothrow_move_constructible_v<EitherImpl<VALUE, ERROR>>)
    requires(!IS_EITHER_LREF)
      : _awaitableEither{std::move(awaitableEither)}
  {
  }

  /// @brief Constructs awaiter from lvalue EitherImpl (holds reference).
  explicit InteropAwaiter(EitherImpl<VALUE, ERROR>& awaitableEither) noexcept
    requires(IS_EITHER_LREF)
      : _awaitableEither{awaitableEither}
  {
  }

  // NOLINTBEGIN(readability-identifier-naming)

  /// @brief Always returns true (never suspends).
  [[nodiscard]]
  auto await_ready() noexcept -> bool
  {
    return true;
  }

  /// @brief Never called since await_ready() always returns true.
  void await_suspend(std::coroutine_handle<>) noexcept {}

  /// @brief No-op for Void data type.
  void await_resume() noexcept
    requires(std::same_as<VALUE, Void>)
  {
  }

  /// @brief Returns reference to EitherImpl (lvalue).
  [[nodiscard]]
  auto await_resume() noexcept -> EitherImpl<VALUE, ERROR>&
    requires(!std::same_as<VALUE, Void> && IS_EITHER_LREF)
  {
    return _awaitableEither;
  }

  /// @brief Returns EitherImpl by move (rvalue).
  [[nodiscard]]
  auto await_resume()
      noexcept(std::is_nothrow_move_constructible_v<EitherImpl<VALUE, ERROR>>)
          -> EitherImpl<VALUE, ERROR>
    requires(!std::same_as<VALUE, Void> && !IS_EITHER_LREF)
  {
    return std::move(_awaitableEither);
  }
  // NOLINTEND(readability-identifier-naming)
};
} // namespace ropic::detail
