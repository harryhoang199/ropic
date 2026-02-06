// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <coroutine>

#include "either_promise.inl"

#include "ropic/void.hpp"

namespace ropic::detail
{

/**
 * @brief Awaiter for Either-to-Either composition with automatic error
 * propagation.
 *
 * Used when co_await-ing an EitherImpl<OTHER, ERROR> inside an
 * EitherImpl<VALUE, ERROR> coroutine. On error: propagates to caller and
 * destroys the coroutine. On success: extracts and returns the data value.
 */
template <typename VALUE, typename ERROR, typename OTHER>
class PropagatingAwaiter
{
  /// @brief Promise type of the caller coroutine (EitherImpl<VALUE, ERROR>).
  using CallerPromise = typename EitherImpl<VALUE, ERROR>::promise_type;

  /// @brief Promise type of the awaited coroutine (EitherImpl<OTHER, ERROR>).
  using AwaitablePromise = typename EitherImpl<OTHER, ERROR>::promise_type;

  /// @brief Reference to the awaited promise for accessing result/error.
  AwaitablePromise& _awaitablePromise;

public:
  /// @brief Constructs awaiter from the awaited coroutine's handle.
  /// @param awaitableHandle Handle to the awaited EitherImpl coroutine.
  explicit PropagatingAwaiter(
      std::coroutine_handle<AwaitablePromise> awaitableHandle)
      : _awaitablePromise{awaitableHandle.promise()}
  {
  }

  // NOLINTBEGIN(readability-identifier-naming)
  /// @brief Returns true if data exists (no suspension needed).
  [[nodiscard]]
  auto await_ready() noexcept -> bool
  {
    return _awaitablePromise.result.has_value();
  }

  /// @brief If error, propagates to caller and destroys the coroutine. If
  /// suspended, propagates suspension state to the returned Either.
  /// @note Always noexcept: only assigns raw pointers (ERROR*), no ERROR
  /// operations.
  void await_suspend(std::coroutine_handle<CallerPromise> h) noexcept
  {
    auto& callerPromise = h.promise();
    if (auto err = _awaitablePromise.error)
    {
      auto currentCont = callerPromise.continuation;
      if (currentCont == nullptr)
      {
        callerPromise.error = _awaitablePromise.error;
      }
      else if (_awaitablePromise.continuation == nullptr)
      {
        while (auto nextCont = currentCont.promise().continuation)
        {
          currentCont = nextCont;
        }
        currentCont.promise().error = err;
      }
      else
      {
        // INVARIANT VIOLATION: Both caller and awaitable have continuations set.
        // This indicates a broken continuation chain state that should never
        // occur in normal execution. If you hit this assert, there may be a bug
        // in the coroutine composition logic or concurrent modification.
        assert(
            false
            && "PropagatingAwaiter: Both caller and awaitable have "
               "continuations - this is an invalid state");
      }

      // reset error of _awaitablePromise to avoid destruction
      _awaitablePromise.error = nullptr;
    }
    else
    {
      _awaitablePromise.continuation =
          std::coroutine_handle<PromiseBase<ERROR>>::from_promise(
              callerPromise);
    }
  }

  /// @brief No-op for Void data type.
  void await_resume() noexcept
    requires(std::is_same_v<OTHER, Void>)
  {
  }

  /// @brief Extracts and moves data value (rvalue).
  [[nodiscard]]
  auto await_resume() noexcept(std::is_nothrow_move_constructible_v<OTHER>)
      -> OTHER
    requires(!std::is_same_v<OTHER, Void>)
  {
    auto& r = _awaitablePromise.result;
    assert(r.has_value() && "EitherImpl must contain data");

    return std::move(r.value());
  }
  // NOLINTEND(readability-identifier-naming)
};
} // namespace ropic::detail
