// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <chrono>
#include <coroutine>
#include <exception>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <utility>

#include "Error.hpp"

#include "ropic/resume_source.hpp"
#include "ropic/safe_awaitable.hpp"

namespace examples
{
// ==========================================
// TASK COROUTINE TYPE
// ==========================================
// A simple lazy SimpleTask coroutine that demonstrates integration with Either.
// SimpleTask<T> represents a deferred computation that produces a value of type
// T.

// C++ coroutines require specific names: promise_type, await_ready, etc.

// NOLINTBEGIN(readability-identifier-naming, readability-convert-member-functions-to-static)

/// @brief Lazy task coroutine that produces a value of type T.
///
/// Demonstrates integration with Either. Suspends on initial_suspend
/// and must be manually resumed via `run()`.
///
/// @tparam T  The result type of the task.
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
    // template <typename VALUE, typename ERROR>
    // auto await_transform(ropic::Either<VALUE, ERROR>& either)
    // {
    //   struct EitherAwaiter
    //   {
    //     ropic::Either<VALUE, ERROR>& either;
    //     bool await_ready() { return !either.error(); }
    //     void await_suspend(std::coroutine_handle<> /*unused*/) {}
    //     auto await_resume() -> VALUE
    //     {
    //       if (auto d = either.value())
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

  explicit SimpleTask(Handle h)
      : handle(h)
  {
  }
  ~SimpleTask()
  {
    if (handle)
      handle.destroy();
  }

  SimpleTask(const SimpleTask&) = delete;
  auto operator=(const SimpleTask&) -> SimpleTask& = delete;
  SimpleTask(SimpleTask&& other) noexcept
      : handle(other.handle)
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

/// @brief Awaitable that simulates an async fetch operation with random
/// latency. Returns the configured string after a random delay (200-1000ms).
class AsyncFetch
{
protected:
  std::string _data;
  std::shared_ptr<std::mutex> _mutex;

public:
  explicit AsyncFetch(std::shared_ptr<std::mutex> mutex) noexcept
      : _mutex(std::move(mutex))
  {
  }

  [[nodiscard]]
  auto await_ready() const noexcept -> bool
  {
    return false;
  }

  [[nodiscard]]
  auto await_suspend(std::coroutine_handle<> h) const noexcept
      -> std::coroutine_handle<>
  {
    // Detach the thread to avoid deadlock: after h.resume() the
    // coroutine continues and may destroy this awaiter while still in
    // the thread. Using detach means the thread runs independently.
    std::thread(
        [h, mutex = _mutex]()
        {
          // Random sleep to simulate variable async latency
          constexpr long kMinSleepMs = 50;
          constexpr long kMaxSleepMs = 100;

          std::random_device rd;
          std::mt19937 gen(rd());
          std::uniform_int_distribution<long> dist(kMinSleepMs, kMaxSleepMs);

          std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
          std::lock_guard otherLock{*mutex};
          h.resume();
        })
        .detach();

    return std::noop_coroutine();
  }

  [[nodiscard]]
  auto await_resume() noexcept -> std::string
  {
    return std::move(_data);
  }

  [[nodiscard]]
  auto operator()(std::string data) -> AsyncFetch&
  {
    _data = std::move(data);
    return *this;
  }
};

/// @brief Variant of AsyncFetch that exposes the suspended coroutine handle
/// to the caller, allowing external control of resumption timing.
class AsyncFetchExposingHandle : public AsyncFetch
{
  std::coroutine_handle<>* _suspendedHandle = nullptr;

  using AsyncFetch::await_suspend;

public:
  explicit AsyncFetchExposingHandle(
      std::shared_ptr<std::mutex> mutex,
      std::coroutine_handle<>* suspendedHandle) noexcept
      : AsyncFetch(std::move(mutex)),
        _suspendedHandle(suspendedHandle)
  {
  }

  auto await_suspend(std::coroutine_handle<> h) const noexcept
      -> std::coroutine_handle<>
  {
    // Detach the thread to avoid deadlock: after h.resume() the coroutine
    // continues and may destroy this awaiter while still in the thread.
    // Using detach means the thread runs independently.
    std::thread(
        [h, suspendedHandle = _suspendedHandle, mutex = _mutex]()
        {
          // Random sleep to simulate variable async latency
          constexpr long kMinSleepMs = 50;
          constexpr long kMaxSleepMs = 100;

          std::random_device rd;
          std::mt19937 gen(rd());
          std::uniform_int_distribution<long> dist(kMinSleepMs, kMaxSleepMs);

          std::this_thread::sleep_for(std::chrono::milliseconds(dist(gen)));
          std::lock_guard otherLock{*mutex};
          *suspendedHandle = h;
        })
        .detach();
    return std::noop_coroutine();
  }

  [[nodiscard]]
  auto operator()(std::string data) -> AsyncFetchExposingHandle&
  {
    _data = std::move(data);
    return *this;
  }
};

/// @brief Variant of AsyncFetchExposingHandle that returns a
/// `coroutine_handle<>` from `await_suspend` for symmetric transfer.
class AsyncFetchWithSymmetricTransfer : public AsyncFetchExposingHandle
{
  std::coroutine_handle<> _nextHandle;

public:
  explicit AsyncFetchWithSymmetricTransfer(
      std::shared_ptr<std::mutex> mutex,
      std::coroutine_handle<>* suspendedHandle,
      std::coroutine_handle<> nextHandle) noexcept
      : AsyncFetchExposingHandle(std::move(mutex), suspendedHandle),
        _nextHandle(nextHandle)
  {
  }

  [[nodiscard]]
  auto await_suspend(std::coroutine_handle<> h) const noexcept
      -> std::coroutine_handle<>
  {
    AsyncFetchExposingHandle::await_suspend(h);
    if (_nextHandle == nullptr || _nextHandle.done())
      return std::noop_coroutine();
    return _nextHandle;
  }

  [[nodiscard]]
  auto operator()(std::string data) -> AsyncFetchWithSymmetricTransfer&
  {
    _data = std::move(data);
    return *this;
  }
};

/// @brief Safe awaitable that receives `ResumeSource` instead of
/// `coroutine_handle<>`, demonstrating three resumption strategies
/// (INLINE, ASYNC, ASYNC_DELAYED).
///
/// @see ropic::safe_awaitable — the concept this type satisfies.
class SafeAsyncFetch
{
  std::string _data;

public:
  enum class ResumeMode : std::uint8_t
  {
    INLINE,
    ASYNC,
    ASYNC_DELAYED
  };

private:
  ResumeMode _mode;

public:
  explicit SafeAsyncFetch(ResumeMode mode) noexcept
      : _mode(mode)
  {
  }

  [[nodiscard]]
  auto await_ready() const noexcept -> bool
  {
    return false;
  }

  // Key difference: receives ResumeSource instead of std::coroutine_handle
  [[nodiscard]]
  auto await_suspend(ropic::ResumeSource resumeSrc) -> bool
  {
    switch (_mode)
    {
    case ResumeMode::INLINE:
      resumeSrc.requestResume();
      return true;

    case ResumeMode::ASYNC:
      std::thread([t = std::move(resumeSrc)]() mutable { t.requestResume(); })
          .detach();
      return true;

    case ResumeMode::ASYNC_DELAYED:
      std::thread(
          [t = std::move(resumeSrc)]() mutable
          {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            t.requestResume();
          })
          .detach();
      return true;
    }
    return true;
  }

  [[nodiscard]]
  auto await_resume() noexcept -> std::string
  {
    return std::move(_data);
  }

  [[nodiscard]]
  auto operator()(std::string data) -> SafeAsyncFetch&
  {
    _data = std::move(data);
    return *this;
  }
};

static_assert(ropic::safe_awaitable<SafeAsyncFetch>);

// NOLINTEND(readability-identifier-naming, readability-convert-member-functions-to-static)

} // namespace examples
