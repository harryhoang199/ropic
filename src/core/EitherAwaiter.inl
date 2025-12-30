// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "Void.hpp"
#include <variant>
#include <coroutine>
#include <cassert>

namespace ropic::detail
{
  /// @brief Checks if a type has a `result` property (used for promise type validation).
  template <typename T>
  concept has_result_property = requires(T &p) {
    { p.result };
  };

  /**
   * @brief Awaiter for Either types in co_await expressions.
   *
   * @tparam AWAITED The data type being awaited
   * @tparam ERROR The error type
   * @tparam PROMISE The promise type (must have a `result` property)
   */
  template <typename AWAITED, typename ERROR, typename PROMISE>
  struct EitherAwaiter
  {
    static_assert(has_result_property<PROMISE>,
                  "`PROMISE` type must have `result` property");

    /// @brief Stores the awaited result: monostate (empty), data, or error.
    std::variant<std::monostate, AWAITED, ERROR> awaitableResult;

    /// @brief Returns true if data is available (resume immediately), false otherwise (suspend).
    [[nodiscard]] auto await_ready() const noexcept -> bool
    {
      return !std::holds_alternative<ERROR>(awaitableResult);
    }

    /// @brief Called when suspending (error case). Moves the error to the promise result.
    void await_suspend(std::coroutine_handle<PROMISE> h) noexcept(std::is_nothrow_move_assignable_v<ERROR>)

    {
      auto *err = std::get_if<ERROR>(&awaitableResult);
      assert(err && "await_suspend called without error state");
      h.promise().result = std::move(*err);
    }

    /// @brief Returns data by value when resuming (success case).
    /// @return The awaited data value (moved).
    [[nodiscard]] auto await_resume() noexcept(std::is_nothrow_move_assignable_v<AWAITED>)
    {
      if constexpr (!std::is_same_v<AWAITED, Void>)
      {
        auto d = std::get_if<AWAITED>(&awaitableResult);
        assert(d && "Either must contain either error or data");
        return std::move(*d);
      }
    }
  };
}