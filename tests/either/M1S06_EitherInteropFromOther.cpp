// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <coroutine>
#include <exception>
#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// =============================================================================
// Simple Task Coroutine Type for Testing Interop
// =============================================================================

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)

/// @brief A minimal Task coroutine type for testing interop scenarios.
template <typename T>
struct SimpleTask
{
  struct Promise
  {
    T result{};
    std::exception_ptr exception;

    [[nodiscard]]
    auto get_return_object() noexcept -> SimpleTask
    {
      return SimpleTask{std::coroutine_handle<Promise>::from_promise(*this)};
    }

    [[nodiscard]]
    auto initial_suspend() noexcept -> std::suspend_never
    {
      return {};
    }

    [[nodiscard]]
    auto final_suspend() noexcept -> std::suspend_always
    {
      return {};
    }

    void return_value(T value) { result = std::move(value); }

    void unhandled_exception() { exception = std::current_exception(); }
  };

  using promise_type = Promise;
  std::coroutine_handle<Promise> handle;

  explicit SimpleTask(std::coroutine_handle<Promise> h)
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

  [[nodiscard]]
  auto result() const -> const T&
  {
    if (handle.promise().exception)
      std::rethrow_exception(handle.promise().exception);
    return handle.promise().result;
  }
};

// =============================================================================
// Test Suite: Other Coroutines Invoking Either-Coroutine
// =============================================================================

TEST(M1S06_EitherInteropFromOther, U01_TaskAwaitsEitherSuccess)
{
  RecordProperty("id", "M1-S06-U01");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Task coroutine co_awaits Either and receives Either object");

  auto task = []() -> SimpleTask<int>
  {
    auto either = co_await returnData(42);
    EXPECT_TRUE(either.done());
    EXPECT_TRUE(either.value());
    co_return *either.value();
  }();

  EXPECT_EQ(task.result(), 42);
}

TEST(M1S06_EitherInteropFromOther, U02_TaskAwaitsEitherError)
{
  RecordProperty("id", "M1-S06-U02");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "Task coroutine co_awaits Either with error");

  auto task = []() -> SimpleTask<std::string>
  {
    auto either = co_await returnError("task error");
    EXPECT_TRUE(either.done());
    EXPECT_TRUE(either.error());
    co_return *either.error();
  }();

  EXPECT_EQ(task.result(), "task error");
}

TEST(M1S06_EitherInteropFromOther, U03_TaskAwaitsMultipleEithers)
{
  RecordProperty("id", "M1-S06-U03");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "Task coroutine co_awaits multiple Either objects");

  auto task = []() -> SimpleTask<int>
  {
    auto e1 = co_await returnData(10);
    auto e2 = co_await returnData(20);
    auto e3 = co_await returnData(30);

    EXPECT_TRUE(e1.done());
    EXPECT_TRUE(e2.done());
    EXPECT_TRUE(e3.done());

    if (e1.error() || e2.error() || e3.error())
      co_return -1;

    co_return *e1.value() + *e2.value() + *e3.value();
  }();

  EXPECT_EQ(task.result(), 60);
}

TEST(M1S06_EitherInteropFromOther, U04_TaskAwaitsEitherMixedResults)
{
  RecordProperty("id", "M1-S06-U04");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Task coroutine handles mixed Either success/error results");

  auto task = []() -> SimpleTask<int>
  {
    auto success = co_await returnData(100);
    auto error = co_await returnError("mixed error");

    EXPECT_TRUE(success.done());
    EXPECT_TRUE(error.done());
    EXPECT_TRUE(success.value());
    EXPECT_TRUE(error.error());

    if (error.error())
      co_return -1;

    co_return *success.value();
  }();

  EXPECT_EQ(task.result(), -1);
}

TEST(M1S06_EitherInteropFromOther, U05_TaskAwaitsEitherVoid)
{
  RecordProperty("id", "M1-S06-U05");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Task coroutine handles Either<Void, Error> correctly");

  // Note: co_await on Either<Void, Error> returns void (no-op via
  // InteropAwaiter) so we cannot use co_await to get the Either back. Instead,
  // we store the Either and check it directly.
  auto task = []() -> SimpleTask<bool>
  {
    auto success = returnOK();
    auto failure = returnVoidError("void error");

    EXPECT_TRUE(success.done());
    EXPECT_TRUE(failure.done());
    EXPECT_FALSE(success.error());
    EXPECT_TRUE(failure.error());

    co_return !success.error() && failure.error();
  }();

  EXPECT_TRUE(task.result());
}

TEST(M1S06_EitherInteropFromOther, U06_TaskAwaitsNestedEither)
{
  RecordProperty("id", "M1-S06-U06");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Task coroutine co_awaits Either from nested Either coroutine");

  auto task = []() -> SimpleTask<int>
  {
    auto result = co_await level1(0);
    EXPECT_TRUE(result.done());
    EXPECT_TRUE(result.value());
    co_return *result.value();
  }();

  EXPECT_EQ(task.result(), 5);
}

TEST(M1S06_EitherInteropFromOther, U07_TaskAwaitsEitherLvalue)
{
  RecordProperty("id", "M1-S06-U07");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "Task coroutine co_awaits lvalue Either reference");

  auto task = []() -> SimpleTask<int>
  {
    auto either = returnData(42);
    // co_await lvalue - should return reference
    auto& ref = co_await either;
    EXPECT_TRUE(ref.done());
    EXPECT_TRUE(ref.value());
    co_return *ref.value();
  }();

  EXPECT_EQ(task.result(), 42);
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
