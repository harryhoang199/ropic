// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <atomic>
#include <chrono>
#include <coroutine>
#include <gtest/gtest.h>
#include <memory>
#include <thread>

#include "TestHelpers.hpp"

// =============================================================================
// Custom Awaiters for Async Testing
// =============================================================================

// NOLINTBEGIN(readability-magic-numbers)

/// @brief A simple awaiter that suspends and stores the coroutine handle.
struct ManualResumeAwaiter
{
  std::coroutine_handle<> handle = nullptr;

  [[nodiscard]]
  static auto await_ready() noexcept -> bool
  {
    return false;
  }

  auto await_suspend(std::coroutine_handle<> h) noexcept -> bool
  {
    handle = h;
    return true;
  }

  static void await_resume() noexcept {}

  void resume()
  {
    if (handle)
      handle.resume();
  }
};

/// @brief An async awaiter that resumes after a delay in a background thread.
/// Uses shared_ptr to ensure proper cleanup even with detached threads.
struct AsyncDelayAwaiter
{
  std::chrono::milliseconds delay;
  std::shared_ptr<std::atomic<bool>> completed;

  explicit AsyncDelayAwaiter(
      std::chrono::milliseconds d,
      std::shared_ptr<std::atomic<bool>> flag = nullptr)
      : delay(d), completed(std::move(flag))
  {
  }

  [[nodiscard]]
  static auto await_ready() noexcept -> bool
  {
    return false;
  }

  void await_suspend(std::coroutine_handle<> h) const
  {
    auto flag = completed;
    std::thread(
        [h, d = delay, flag]
        {
          std::this_thread::sleep_for(d);
          h.resume();
          if (flag)
            flag->store(true);
        })
        .detach();
  }

  static void await_resume() noexcept {}
};

/// @brief An async awaiter that returns a value after delay.
template <typename T>
struct AsyncValueAwaiter
{
  std::chrono::milliseconds delay;
  T value;

  AsyncValueAwaiter(std::chrono::milliseconds d, T v)
      : delay(d), value(std::move(v))
  {
  }

  [[nodiscard]]
  static auto await_ready() noexcept -> bool
  {
    return false;
  }

  void await_suspend(std::coroutine_handle<> h) const
  {
    std::thread(
        [h, d = delay]
        {
          std::this_thread::sleep_for(d);
          h.resume();
        })
        .detach();
  }

  [[nodiscard]]
  auto await_resume() -> T
  {
    return std::move(value);
  }
};

/// @brief Helper to poll for completion with timeout.
template <typename F>
auto pollUntilDone(F&& isDone, std::chrono::milliseconds timeout) -> bool
{
  auto start = std::chrono::steady_clock::now();
  while (!isDone())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (std::chrono::steady_clock::now() - start > timeout)
      return false;
  }
  return true;
}

// =============================================================================
// Test Suite: Async Function Tests with done() Behavior
// =============================================================================

TEST(EitherAsyncBehavior, UNIT_042_DoneReturnsFalseWhileSuspended)
{
  RecordProperty("id", "0.02-UNIT-042");
  RecordProperty("desc", "done() returns false while coroutine is suspended");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 42;
  };

  auto either = coro();

  EXPECT_FALSE(either.done()) << "done() should return false while suspended";
  EXPECT_FALSE(either.data())
      << "data() should return empty Borrower while suspended";
  EXPECT_FALSE(either.error())
      << "error() should return empty Borrower while suspended";

  awaiter.resume();

  EXPECT_TRUE(either.done()) << "done() should return true after resume";
  EXPECT_TRUE(either.data())
      << "data() should return valid Borrower after resume";
}

TEST(EitherAsyncBehavior, UNIT_043_AsyncDelayDonePolling)
{
  RecordProperty("id", "0.02-UNIT-043");
  RecordProperty("desc", "done() correctly reflects async completion state");

  auto completed = std::make_shared<std::atomic<bool>>(false);

  auto coro = [completed]() -> Either<int, std::string>
  {
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(50), completed};
    co_return 123;
  };

  auto either = coro();

  EXPECT_FALSE(either.done())
      << "done() should return false immediately after creation";

  ASSERT_TRUE(
      pollUntilDone([&] { return either.done(); }, std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.data());
  EXPECT_EQ(*either.data(), 123);
}

TEST(EitherAsyncBehavior, UNIT_044_AsyncDelayWithError)
{
  RecordProperty("id", "0.02-UNIT-044");
  RecordProperty("desc", "Async Either correctly reports error after delay");

  auto coro = []() -> Either<int, std::string>
  {
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30)};
    co_return std::string("async error");
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  ASSERT_TRUE(
      pollUntilDone([&] { return either.done(); }, std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.error());
  EXPECT_EQ(*either.error(), "async error");
}

TEST(EitherAsyncBehavior, UNIT_045_AsyncWithValueAwaiter)
{
  RecordProperty("id", "0.02-UNIT-045");
  RecordProperty(
      "desc", "Async Either receives value from async awaiter and computes");

  auto coro = []() -> Either<int, std::string>
  {
    int value =
        co_await AsyncValueAwaiter<int>{std::chrono::milliseconds(20), 100};
    co_return value * 2;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  ASSERT_TRUE(
      pollUntilDone([&] { return either.done(); }, std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.data());
  EXPECT_EQ(*either.data(), 200);
}

TEST(EitherAsyncBehavior, UNIT_046_AsyncChainedOperations)
{
  RecordProperty("id", "0.02-UNIT-046");
  RecordProperty("desc", "Async Either with multiple async awaiters in chain");

  auto coro = []() -> Either<int, std::string>
  {
    int v1 = co_await AsyncValueAwaiter<int>{std::chrono::milliseconds(10), 10};
    int v2 = co_await AsyncValueAwaiter<int>{std::chrono::milliseconds(10), 20};
    co_return v1 + v2;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  ASSERT_TRUE(
      pollUntilDone([&] { return either.done(); }, std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.data());
  EXPECT_EQ(*either.data(), 30);
}

TEST(EitherAsyncBehavior, UNIT_047_AsyncMixedWithEitherPropagation)
{
  RecordProperty("id", "0.02-UNIT-047");
  RecordProperty(
      "desc",
      "Async Either mixes async awaiters with Either error propagation");

  auto coro = []() -> Either<int, std::string>
  {
    int v = co_await AsyncValueAwaiter<int>{std::chrono::milliseconds(15), 50};
    int x = co_await returnData(v);
    co_return x + 5;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  ASSERT_TRUE(
      pollUntilDone([&] { return either.done(); }, std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.data());
  EXPECT_EQ(*either.data(), 55);
}

TEST(EitherAsyncBehavior, UNIT_048_AsyncErrorPropagationAfterDelay)
{
  RecordProperty("id", "0.02-UNIT-048");
  RecordProperty("desc", "Async Either propagates error after async operation");

  auto coro = []() -> Either<int, std::string>
  {
    [[maybe_unused]]
    int v = co_await AsyncValueAwaiter<int>{std::chrono::milliseconds(15), 50};
    int x = co_await returnError("propagated after async");
    co_return x;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  ASSERT_TRUE(
      pollUntilDone([&] { return either.done(); }, std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.error());
  EXPECT_EQ(*either.error(), "propagated after async");
}

// NOLINTEND(readability-magic-numbers)
