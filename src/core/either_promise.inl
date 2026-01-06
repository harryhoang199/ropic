// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <coroutine>
#include <exception>

#include "either_impl.hpp"

namespace ropic::detail
{

/**
 * @brief Promise type for Either coroutines.
 *
 * Controls coroutine lifecycle: immediate start, no final suspend,
 * and stores co_return values directly into the associated EitherImpl.
 */
template <typename DATA, typename ERROR>
class EitherImpl<DATA, ERROR>::Promise
{
  EitherImpl* _either = nullptr;

public:
  using DataType = DATA;

  /// @brief Binds this promise to its owning EitherImpl instance.
  void setEither(EitherImpl* either) noexcept
  {
    assert(either);
    _either = either;
  }

  // NOLINTBEGIN(readability-identifier-naming)
  /// @brief Creates EitherImpl bound to this promise's coroutine handle.
  [[nodiscard]]
  auto get_return_object() noexcept -> EitherImpl
  {
    return EitherImpl{Handle::from_promise(*this)};
  }

  /// @brief Starts execution immediately (no initial suspend).
  [[nodiscard]]
  auto initial_suspend() noexcept -> std::suspend_never
  {
    return {};
  }

  /// @brief Handles co_return with a DATA value.
  void return_value(DATA value)
      noexcept(std::is_nothrow_move_assignable_v<DATA>)
  {
    _either->_setDataAndNullifyHandle(std::move(value));
  }

  /// @brief Handles co_return with an ERROR value.

  void return_value(ERROR value)
      noexcept(std::is_nothrow_move_assignable_v<ERROR>)
  {
    _either->_setErrorAndNullifyHandle(std::move(value));
  }

  /// @brief Suspends at coroutine end.
  [[nodiscard]]
  auto final_suspend() noexcept -> std::suspend_never
  {
    return {};
  }

  /// @brief Terminates on unhandled exceptions.
  void unhandled_exception() noexcept { std::terminate(); }

  /// @brief Pass-through for non-Either awaitables.
  template <typename T>
  auto await_transform(T&& awaitable) -> T&&
  {
    return static_cast<T&&>(awaitable);
  }

  /// @brief Transforms rvalue EitherImpl to PropagatingAwaiter for error
  /// propagation.
  template <typename OTHER>
  auto await_transform(EitherImpl<OTHER, ERROR>&& awaitable)
      -> PropagatingAwaiter<OTHER, false>
  {
    return PropagatingAwaiter<OTHER, false>{std::move(awaitable), *_either};
  }

  /// @brief Transforms lvalue EitherImpl to PropagatingAwaiter for error
  /// propagation.
  template <typename OTHER>
  auto await_transform(EitherImpl<OTHER, ERROR>& awaitable)
      -> PropagatingAwaiter<OTHER, true>
  {
    return PropagatingAwaiter<OTHER, true>{awaitable, *_either};
  }
  // NOLINTEND(readability-identifier-naming)
};
} // namespace ropic::detail
