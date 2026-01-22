// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <coroutine>
#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// =============================================================================
// Custom Awaiters for Testing
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

/// @brief An awaiter that returns a value after manual resume.
template <typename T>
struct ManualResumeAwaiterWithValue
{
  std::coroutine_handle<> handle = nullptr;
  T value;

  explicit ManualResumeAwaiterWithValue(T v) : value(std::move(v)) {}

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

// =============================================================================
// Test Suite: Either-Coroutine Invoking Other Coroutines
// =============================================================================

TEST(EitherInteropToOther, UNIT_036_EitherAwaitsCustomAwaiter)
{
  RecordProperty("id", "0.02-UNIT-036");
  RecordProperty("desc", "Either coroutine co_awaits custom awaiter");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 42;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());
  EXPECT_FALSE(either.data());
  EXPECT_FALSE(either.error());

  awaiter.resume();

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.data());
  EXPECT_EQ(*either.data(), 42);
}

TEST(EitherInteropToOther, UNIT_037_EitherAwaitsValueAwaiter)
{
  RecordProperty("id", "0.02-UNIT-037");
  RecordProperty("desc", "Either coroutine receives value from custom awaiter");

  ManualResumeAwaiterWithValue<int> awaiter{100};

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    int value = co_await awaiter;
    co_return value + 5;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  awaiter.resume();

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.data());
  EXPECT_EQ(*either.data(), 105);
}

TEST(EitherInteropToOther, UNIT_038_EitherAwaitsMultipleAwaiters)
{
  RecordProperty("id", "0.02-UNIT-038");
  RecordProperty(
      "desc",
      "Either coroutine co_awaits multiple custom awaiters in sequence");

  ManualResumeAwaiterWithValue<int> awaiter1{10};
  ManualResumeAwaiterWithValue<int> awaiter2{20};

  auto coro = [&awaiter1, &awaiter2]() -> Either<int, std::string>
  {
    int v1 = co_await awaiter1;
    int v2 = co_await awaiter2;
    co_return v1 + v2;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  awaiter1.resume();
  EXPECT_FALSE(either.done());

  awaiter2.resume();
  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.data());
  EXPECT_EQ(*either.data(), 30);
}

TEST(EitherInteropToOther, UNIT_039_EitherAwaitsMixedAwaitables)
{
  RecordProperty("id", "0.02-UNIT-039");
  RecordProperty(
      "desc",
      "Either coroutine mixes custom awaiters with Either error propagation");

  ManualResumeAwaiterWithValue<int> awaiter{50};

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    int v = co_await awaiter;
    int x = co_await returnData(v);
    co_return x + 10;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  awaiter.resume();

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.data());
  EXPECT_EQ(*either.data(), 60);
}

TEST(EitherInteropToOther, UNIT_040_EitherAwaitsCustomThenPropagatesError)
{
  RecordProperty("id", "0.02-UNIT-040");
  RecordProperty(
      "desc",
      "Either coroutine awaits custom awaiter then propagates Either error");

  ManualResumeAwaiterWithValue<int> awaiter{50};

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    [[maybe_unused]]
    int v = co_await awaiter;
    int x = co_await returnError("after custom");
    co_return x;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  awaiter.resume();

  EXPECT_TRUE(either.done());
  EXPECT_TRUE(either.error());
  EXPECT_EQ(*either.error(), "after custom");
}

TEST(EitherInteropToOther, UNIT_041_EitherAwaitsVoidAwaiter)
{
  RecordProperty("id", "0.02-UNIT-041");
  RecordProperty(
      "desc", "Either<Void, Error> coroutine co_awaits custom awaiter");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<Void, std::string>
  {
    co_await awaiter;
    co_return OK;
  };

  auto either = coro();

  EXPECT_FALSE(either.done());

  awaiter.resume();

  EXPECT_TRUE(either.done());
  EXPECT_FALSE(either.error());
}

// NOLINTEND(readability-magic-numbers)
