// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <coroutine>

namespace ropic::detail
{

/**
 * @brief Final awaiter for coroutine suspension with symmetric transfer.
 *
 * Returns from `await_suspend()` the continuation handle, enabling symmetric
 * transfer (tail-call optimization) to the next coroutine without stack growth.
 * If continuation is `std::noop_coroutine()`, the coroutine frame is destroyed.
 *
 * @see Promise::final_suspend() which creates this awaiter.
 */
class FinalAwaiter
{
  /// @brief Handle to resume after this coroutine completes.
  std::coroutine_handle<> _continuation;

public:
  /// @brief Constructs with the continuation to resume.
  /// @param continuation Handle to next coroutine, or noop_coroutine() to end.
  explicit FinalAwaiter(std::coroutine_handle<> continuation) noexcept
      : _continuation(continuation)
  {
  }

  // NOLINTBEGIN(readability-identifier-naming, readability-convert-member-functions-to-static)

  /// @brief Always returns false to trigger await_suspend for symmetric
  /// transfer.
  [[nodiscard]]
  auto await_ready() noexcept -> bool
  {
    return false;
  }

  /// @brief Returns continuation handle for symmetric transfer.
  /// @return The continuation handle; runtime resumes it (or destroys if noop).
  [[nodiscard]]
  auto await_suspend(std::coroutine_handle<>) const noexcept
      -> std::coroutine_handle<>
  {
    return _continuation;
  }

  /// @brief No-op. Final awaiter doesn't produce a value.
  void await_resume() const noexcept {}

  // NOLINTEND(readability-identifier-naming, readability-convert-member-functions-to-static)
};
} // namespace ropic::detail
