
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "either_impl.hpp"
#include "void.hpp"

namespace ropic::detail
{
/**
 * @brief Awaiter for Either-to-Either composition with automatic error
 * propagation.
 *
 * Used when co_await-ing an EitherImpl<OTHER, ERROR> inside an EitherImpl<DATA,
 * ERROR> coroutine. On error: propagates to caller and destroys the coroutine.
 * On success: extracts and returns the data value.
 */
template <typename DATA, typename ERROR>
template <typename OTHER, bool IS_LVALUE>
class EitherImpl<DATA, ERROR>::PropagatingAwaiter
{
  AwaitableEither<OTHER, IS_LVALUE> _awaitableEither;
  EitherImpl& _returnEither;

public:
  explicit PropagatingAwaiter(
      EitherImpl<OTHER, ERROR>&& awaitableEither, EitherImpl& returnEither)
      noexcept(std::is_nothrow_move_assignable_v<EitherImpl<OTHER, ERROR>>)
    requires(!IS_LVALUE)
      : _awaitableEither{std::move(awaitableEither)},
        _returnEither{returnEither}
  {
  }

  explicit PropagatingAwaiter(
      EitherImpl<OTHER, ERROR>& awaitableEither,
      EitherImpl& returnEither) noexcept
    requires(IS_LVALUE)
      : _awaitableEither{awaitableEither}, _returnEither{returnEither}
  {
  }

  // NOLINTBEGIN(readability-identifier-naming)
  /// @brief Returns true if data exists (no suspension needed).
  [[nodiscard]]
  auto await_ready() noexcept -> bool
  {
    return static_cast<bool>(_awaitableEither.data());
  }

  /// @brief Propagates error to caller and destroys the coroutine.
  void await_suspend(std::coroutine_handle<Promise> h)
      noexcept(std::is_nothrow_move_assignable_v<ERROR>)
  {
    auto err = _awaitableEither.error();
    assert(err && "`await_suspend` must be called with error state");

    _returnEither._setErrorAndNullifyHandle(std::move(*err));
    h.destroy();
  }

  /// @brief No-op for Void data type.
  void await_resume() noexcept
    requires(std::is_same_v<OTHER, Void>)
  {
  }

  /// @brief Extracts and moves data value (rvalue).
  [[nodiscard]]
  auto await_resume() noexcept(std::is_nothrow_move_assignable_v<OTHER>)
      -> OTHER
    requires(!std::is_same_v<OTHER, Void> && !IS_LVALUE)
  {
    auto d = _awaitableEither.data();
    assert(d && "EitherImpl must contain data");

    return std::move(*d);
  }

  /// @brief Returns reference to data value (lvalue).
  [[nodiscard]]
  auto await_resume() noexcept -> OTHER&
    requires(!std::is_same_v<OTHER, Void> && IS_LVALUE)
  {
    auto d = _awaitableEither.data();
    assert(d && "EitherImpl must contain data");

    return *d;
  }
  // NOLINTEND(readability-identifier-naming)
};

/**
 * @brief Awaiter for interoperability with non-Either coroutines (Task,
 * Generator, etc.).
 *
 * Used when co_await-ing an Either from outside the Either coroutine
 * ecosystem. Always ready (never suspends). Returns the EitherImpl object
 * itself without unwrapping.
 */
template <typename DATA, typename ERROR>
template <bool IS_LVALUE>
class EitherImpl<DATA, ERROR>::InteropAwaiter
{

  AwaitableEither<DATA, IS_LVALUE> _awaitableEither;

public:
  explicit InteropAwaiter(EitherImpl&& awaitableEither)
      noexcept(std::is_nothrow_move_assignable_v<EitherImpl>)
    requires(!IS_LVALUE)
      : _awaitableEither{std::move(awaitableEither)}
  {
  }

  explicit InteropAwaiter(EitherImpl& awaitableEither) noexcept
    requires(IS_LVALUE)
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
    requires(std::is_same_v<DATA, Void>)
  {
  }

  /// @brief Returns reference to EitherImpl (lvalue).
  [[nodiscard]]
  auto await_resume() noexcept -> EitherImpl&
    requires(!std::is_same_v<DATA, Void> && IS_LVALUE)
  {
    return _awaitableEither;
  }

  /// @brief Returns EitherImpl by move (rvalue).
  [[nodiscard]]
  auto await_resume() noexcept(std::is_nothrow_move_assignable_v<DATA>)
      -> EitherImpl
    requires(!std::is_same_v<DATA, Void> && !IS_LVALUE)
  {
    return std::move(_awaitableEither);
  }

  // NOLINTEND(readability-identifier-naming)
};
} // namespace ropic::detail
