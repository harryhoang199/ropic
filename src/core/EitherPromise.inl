// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "Void.hpp"
#include "EitherAwaiter.inl"
#include <cassert>

namespace ropic::detail
{
  /**
   * @brief Promise type for Either coroutines. Controls initialization, return handling, and suspension.
   *
   * @tparam DATA The success value type
   * @tparam ERROR The error type
   * @tparam EITHER The Either template type (e.g., Either<DATA, ERROR>)
   */
  template <typename DATA,
            typename ERROR,
            template <typename, typename> typename EITHER>
  struct EitherPromise
  {
    std::variant<std::monostate, DATA, ERROR> result; ///< Stores monostate, data, or error

    /// @brief Creates Either bound to this promise's coroutine handle.
    [[nodiscard]] auto get_return_object() noexcept -> EITHER<DATA, ERROR>
      requires std::constructible_from<EITHER<DATA, ERROR>, std::coroutine_handle<EitherPromise>>
    {
      return EITHER{std::coroutine_handle<EitherPromise>::from_promise(*this)};
    }

    /// @brief Starts execution immediately (no initial suspend).
    [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_never { return {}; }

    /// @brief Handles co_return with a DATA value.
    void return_value(DATA value) noexcept(std::is_nothrow_move_assignable_v<DATA>)
    {
      result = std::move(value);
    }

    /// @brief Handles co_return with an ERROR value.
    void return_value(ERROR value) noexcept(std::is_nothrow_move_assignable_v<ERROR>)
    {
      result = std::move(value);
    }

    /// @brief Suspends at coroutine end.
    [[nodiscard]] auto final_suspend() noexcept -> std::suspend_always { return {}; }

    /// @brief Terminates on unhandled exceptions.
    void unhandled_exception() noexcept { std::terminate(); }

    /// @brief Transforms co_await expressions for Either types.
    template <typename AWAITED>
    [[nodiscard]] auto await_transform(EITHER<AWAITED, ERROR> awaitable) noexcept
        -> EitherAwaiter<AWAITED, ERROR, EitherPromise>
    {
      if (auto err = awaitable.error())
        return EitherAwaiter<AWAITED, ERROR, EitherPromise>{std::move(*err)};

      if constexpr (std::is_same_v<AWAITED, Void>)
        return EitherAwaiter<AWAITED, ERROR, EitherPromise>{};
      else
      {
        auto d = awaitable.data();
        assert(d && "Either must contain either error or data");
        return EitherAwaiter<AWAITED, ERROR, EitherPromise>{std::move(*d)};
      }
    }
  };
}