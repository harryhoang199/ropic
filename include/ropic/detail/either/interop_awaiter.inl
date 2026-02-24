// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <coroutine>

#include "ropic/void.hpp"

#include "ropic/detail/either/either_impl.hpp"

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
  /// @param awaitableEither  The EitherImpl to move into this awaiter.
  explicit InteropAwaiter(EitherImpl<VALUE, ERROR>&& awaitableEither)
      noexcept(std::is_nothrow_move_constructible_v<EitherImpl<VALUE, ERROR>>)
    requires(!IS_EITHER_LREF)
      : _awaitableEither{std::move(awaitableEither)}
  {
  }

  /// @brief Constructs awaiter from lvalue EitherImpl (holds reference).
  /// @param awaitableEither  The EitherImpl to reference.
  explicit InteropAwaiter(EitherImpl<VALUE, ERROR>& awaitableEither) noexcept
    requires(IS_EITHER_LREF)
      : _awaitableEither{awaitableEither}
  {
  }

  // NOLINTBEGIN(readability-identifier-naming)

  /// @brief Always returns true (never suspends).
  /// @return Always `true`.
  [[nodiscard]]
  auto await_ready() noexcept -> bool
  {
    return true;
  }

  /// @brief Never called since await_ready() always returns true.
  void await_suspend(std::coroutine_handle<>) noexcept {}

  /// @brief Returns the EitherImpl to the caller.
  /// @return `EitherImpl&` for lvalue, `EitherImpl&&` for rvalue,
  ///         or `void` if VALUE is Void.
  [[nodiscard]]
  auto await_resume() noexcept -> decltype(auto)
  {
    if constexpr (std::same_as<VALUE, Void>)
      return;
    else if constexpr (IS_EITHER_LREF)
      return (_awaitableEither);
    else
      return std::move(_awaitableEither);
  }

  // NOLINTEND(readability-identifier-naming)
};
} // namespace ropic::detail
