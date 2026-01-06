// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <optional>
#include <random>
#include <thread>
#include <utility>

#include "Error.hpp"

namespace examples
{
// ==========================================
// TASK COROUTINE TYPE
// ==========================================
// A simple lazy SimpleTask coroutine that demonstrates integration with Either.
// SimpleTask<T> represents a deferred computation that produces a value of type
// T.

// NOLINTBEGIN(readability-identifier-naming)
// C++ coroutines require specific names: promise_type, await_ready, etc.

template <typename T>
struct SimpleTask
{
  struct promise_type
  {
    std::optional<T> result;

    auto get_return_object() { return SimpleTask{Handle::from_promise(*this)}; }
    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }
    void unhandled_exception()
    {
      auto throwedException = std::current_exception();
      if (throwedException)
      {
        try
        {
          std::rethrow_exception(throwedException);
        }
        catch (Error const& e)
        {
          std::cout << "\nRethrowed exception: " << e.message() << "\n";
        }
        catch (...)
        {
          std::cout << "An unknown exception throwed\n";
          throw;
        }
      }
      else
      {
        std::cout << "An unknown exception throwed\n";
        throw;
      }
    }
    void return_value(T value) { result = std::move(value); }

    // // Enable co_await on Either inside SimpleTask coroutines
    // template <typename DATA, typename ERROR>
    // auto await_transform(ropic::Either<DATA, ERROR>& either)
    // {
    //   struct EitherAwaiter
    //   {
    //     ropic::Either<DATA, ERROR>& either;
    //     bool await_ready() { return !either.error(); }
    //     void await_suspend(std::coroutine_handle<> /*unused*/) {}
    //     auto await_resume() -> DATA
    //     {
    //       if (auto d = either.data())
    //         return std::move(*d);
    //       // In a real implementation, should handle the error appropriately
    //       std::terminate();
    //     }
    //   };
    //   return EitherAwaiter{either};
    // }
  };

  using Handle = std::coroutine_handle<promise_type>;
  Handle handle;

  explicit SimpleTask(Handle h) : handle(h) {}
  ~SimpleTask()
  {
    if (handle)
      handle.destroy();
  }

  SimpleTask(const SimpleTask&) = delete;
  auto operator=(const SimpleTask&) -> SimpleTask& = delete;
  SimpleTask(SimpleTask&& other) noexcept : handle(other.handle)
  {
    other.handle = nullptr;
  }
  auto operator=(SimpleTask&& other) noexcept -> SimpleTask&
  {
    if (this != &other)
    {
      if (handle)
        handle.destroy();
      handle = other.handle;
      other.handle = nullptr;
    }
    return *this;
  }

  // Run the task to completion and get the result
  auto run() -> T
  {
    handle.resume();
    return std::move(*handle.promise().result);
  }
};

/// @brief Awaitable that simulates an async fetch operation with random latency.
/// Returns the configured string after a random delay (200-1000ms).
class AsyncFetch
{
  std::string _data;

public:
  explicit AsyncFetch(std::string returnData) noexcept
      : _data(std::move(returnData))
  {
  }

  auto await_ready() -> bool { return false; }
  auto await_suspend(std::coroutine_handle<> h) -> bool
  {
    // Detach the thread to avoid deadlock: after h.resume() the coroutine
    // continues and may destroy this awaiter while still in the thread.
    // Using detach means the thread runs independently.
    std::thread(
        [h]()
        {
          // Random sleep to simulate variable async latency
          constexpr long kMinSleepMs = 200;
          constexpr long kMaxSleepMs = 1000;

          std::random_device rd;
          std::mt19937 gen(rd());
          std::uniform_int_distribution<long> dist(kMinSleepMs, kMaxSleepMs);

          std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
          h.resume();
        })
        .detach();

    return true;
  }
  auto await_resume() -> std::string { return std::move(_data); }
};

// NOLINTEND(readability-identifier-naming)

} // namespace examples
