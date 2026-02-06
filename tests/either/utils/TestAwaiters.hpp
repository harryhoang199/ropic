// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <chrono>
#include <coroutine>
#include <memory>
#include <mutex>
#include <thread>

// =============================================================================
// Custom Awaiters for Async Testing
// =============================================================================

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)

/// @brief A simple awaiter that suspends and stores the coroutine handle.
/// Used to test move semantics with suspended coroutines.
struct ManualResumeAwaiter
{
  std::coroutine_handle<> handle = nullptr;

  [[nodiscard]]
  auto await_ready() const noexcept -> bool
  {
    return false;
  }

  auto await_suspend(std::coroutine_handle<> h) noexcept -> bool
  {
    handle = h;
    return true;
  }

  void await_resume() const noexcept {}

  void resume() const
  {
    if (handle)
      handle.resume();
  }
};

/// @brief An awaiter that returns a value after manual resume.
template <typename T>
struct ManualResumeAwaiterWithValue
{
  std::coroutine_handle<> handle = nullptr;
  T value;

  explicit ManualResumeAwaiterWithValue(T v)
      : value(std::move(v))
  {
  }

  [[nodiscard]]
  auto await_ready() const noexcept -> bool
  {
    return false;
  }

  auto await_suspend(std::coroutine_handle<> h) noexcept -> bool
  {
    handle = h;
    return true;
  }

  [[nodiscard]]
  auto await_resume() noexcept -> T
  {
    return std::move(value);
  }

  void resume()
  {
    if (handle)
      handle.resume();
  }
};

/// @brief An async awaiter that resumes after a delay in a background thread.
/// Uses mutex to synchronize resume with coroutine state access.
struct AsyncDelayAwaiter
{
  std::chrono::milliseconds _delay;
  std::shared_ptr<std::mutex> _mutex;

  explicit AsyncDelayAwaiter(
      std::chrono::milliseconds delay, std::shared_ptr<std::mutex> mutex)
      : _delay(delay),
        _mutex(std::move(mutex))
  {
  }

  [[nodiscard]]
  auto await_ready() const noexcept -> bool
  {
    return false;
  }

  [[nodiscard]]
  auto await_suspend(std::coroutine_handle<> h) const -> bool
  {
    std::thread(
        [h, d = _delay, mutex = _mutex]
        {
          std::this_thread::sleep_for(d);
          std::lock_guard lock{*mutex};
          h.resume();
        })
        .detach();

    return true;
  }

  void await_resume() const noexcept {}
};

/// @brief An async awaiter that returns a value after delay.
/// Uses mutex to synchronize resume with coroutine state access.
template <typename T>
struct AsyncValueAwaiter
{
  std::chrono::milliseconds _delay;
  T _value;
  std::shared_ptr<std::mutex> _mutex;

  AsyncValueAwaiter(
      std::chrono::milliseconds delay,
      T value,
      std::shared_ptr<std::mutex> mutex)
      : _delay(delay),
        _value(std::move(value)),
        _mutex(std::move(mutex))
  {
  }

  [[nodiscard]]
  auto await_ready() const noexcept -> bool
  {
    return false;
  }

  auto await_suspend(std::coroutine_handle<> h) -> bool
  {
    std::thread(
        [h, d = _delay, mutex = _mutex]
        {
          std::this_thread::sleep_for(d);
          std::lock_guard lock{*mutex};
          h.resume();
        })
        .detach();

    return true;
  }

  [[nodiscard]]
  auto await_resume() -> T
  {
    return std::move(_value);
  }
};

// =============================================================================
// Polling Utilities
// =============================================================================

/// @brief Helper to poll for completion with timeout.
template <typename F>
auto pollUntilDone(
    F&& isDone,
    std::chrono::milliseconds timeout = std::chrono::milliseconds(-1)) -> bool
{
  auto start = std::chrono::steady_clock::now();
  if (timeout < std::chrono::milliseconds(0))
  {
    while (!isDone())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  }
  else
  {
    while (!isDone())
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));

      if (std::chrono::steady_clock::now() - start > timeout)
        return false;
    }
  }
  return true;
}
// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
