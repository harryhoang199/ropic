// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include <memory>
#include <mutex>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)

// =============================================================================
// Test Suite: Async Function Tests with done() Behavior
// =============================================================================

TEST(M1S08_EitherShallowAsyncBehavior, U01_SuspendedWithoutError)
{
  RecordProperty("id", "M1-S08-U01");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Succeeded Either's done() returns false while coroutine is suspended");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 42;
  };

  auto either = coro();

  EXPECT_FALSE(either.done()) << "done() should return false while suspended";
  EXPECT_FALSE(either.value())
      << "value() should return empty Borrower while suspended";
  EXPECT_FALSE(either.error())
      << "error() should return empty Borrower while suspended";

  awaiter.resume();

  EXPECT_TRUE(either.done()) << "done() should return true after resume";
  EXPECT_EQ(*either.value(), 42)
      << "value() should return valid Borrower after resume";
}

TEST(M1S08_EitherShallowAsyncBehavior, U02_SuspendedWithError)
{
  RecordProperty("id", "M1-S08-U02");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Failed Either's done() returns false while coroutine is suspended");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return std::string("ERROR 404");
  };

  auto either = coro();

  EXPECT_FALSE(either.done()) << "done() should return false while suspended";
  EXPECT_FALSE(either.value())
      << "value() should return empty Borrower while suspended";
  EXPECT_FALSE(either.error())
      << "error() should return empty Borrower while suspended";

  awaiter.resume();

  EXPECT_TRUE(either.done()) << "done() should return true after resume";
  EXPECT_EQ(*either.error(), std::string("ERROR 404"))
      << "value() should return valid Borrower after resume";
}

TEST(M1S08_EitherShallowAsyncBehavior, U03_AsyncDelayDonePolling)
{
  RecordProperty("id", "M1-S08-U03");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "done() correctly reflects async completion state");

  auto mutex = std::make_shared<std::mutex>();

  auto coro = [mutex]() -> Either<int, std::string>
  {
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(50), mutex};
    co_return 123;
  };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(coro());
    EXPECT_FALSE(eitherPtr->done())
        << "done() should return false immediately after creation";
  }

  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->done();
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(eitherPtr->done());
  EXPECT_TRUE(eitherPtr->value());
  EXPECT_EQ(*(eitherPtr->value()), 123);
  delete eitherPtr;
}

TEST(M1S08_EitherShallowAsyncBehavior, U04_AsyncDelayWithError)
{
  RecordProperty("id", "M1-S08-U04");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "Async Either correctly reports error after delay");

  auto mutex = std::make_shared<std::mutex>();

  auto coro = [mutex]() -> Either<int, std::string>
  {
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30), mutex};
    co_return std::string("async error");
  };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(coro());
    EXPECT_FALSE(eitherPtr->done())
        << "done() should return false immediately after creation";
  }

  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->done();
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(eitherPtr->done());
  EXPECT_TRUE(eitherPtr->error());
  EXPECT_EQ(*eitherPtr->error(), "async error");
  delete eitherPtr;
}

TEST(M1S08_EitherShallowAsyncBehavior, U05_AsyncWithValueAwaiter)
{
  RecordProperty("id", "M1-S08-U05");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Async Either receives value from async awaiter and computes");

  auto mutex = std::make_shared<std::mutex>();

  auto coro = [mutex]() -> Either<int, std::string>
  {
    int value = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(20), 100, mutex};
    co_return value * 2;
  };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(coro());
    EXPECT_FALSE(eitherPtr->done())
        << "done() should return false immediately after creation";
  }

  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->done();
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(eitherPtr->done());
  EXPECT_TRUE(eitherPtr->value());
  EXPECT_EQ(*eitherPtr->value(), 200);
  delete eitherPtr;
}

TEST(M1S08_EitherShallowAsyncBehavior, U06_AsyncChainedOperations)
{
  RecordProperty("id", "M1-S08-U06");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "Async Either with multiple async awaiters in chain");

  auto mutex = std::make_shared<std::mutex>();

  auto coro = [mutex]() -> Either<int, std::string>
  {
    int v1 = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(10), 10, mutex};
    int v2 = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(10), 20, mutex};
    co_return v1 + v2;
  };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(coro());
    EXPECT_FALSE(eitherPtr->done())
        << "done() should return false immediately after creation";
  }

  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->done();
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(eitherPtr->done());
  EXPECT_TRUE(eitherPtr->value());
  EXPECT_EQ(*eitherPtr->value(), 30);
  delete eitherPtr;
}

TEST(M1S08_EitherShallowAsyncBehavior, U07_AsyncMixedWithEitherPropagation)
{
  RecordProperty("id", "M1-S08-U07");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Async Either mixes async awaiters with Either error propagation");

  auto mutex = std::make_shared<std::mutex>();

  auto coro = [mutex]() -> Either<int, std::string>
  {
    int v = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(15), 50, mutex};
    int x = co_await returnData(v);
    co_return x + 5;
  };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(coro());
    EXPECT_FALSE(eitherPtr->done())
        << "done() should return false immediately after creation";
  }

  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->done();
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(eitherPtr->done());
  EXPECT_TRUE(eitherPtr->value());
  EXPECT_EQ(*eitherPtr->value(), 55);
  delete eitherPtr;
}

TEST(M1S08_EitherShallowAsyncBehavior, U08_AsyncErrorPropagationAfterDelay)
{
  RecordProperty("id", "M1-S08-U08");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "Async Either propagates error after async operation");

  auto mutex = std::make_shared<std::mutex>();

  auto coro = [mutex]() -> Either<int, std::string>
  {
    [[maybe_unused]]
    int v = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(15), 50, mutex};
    int x = co_await returnError("propagated after async");
    co_return x;
  };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(coro());
    EXPECT_FALSE(eitherPtr->done())
        << "done() should return false immediately after creation";
  }

  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->done();
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(eitherPtr->done());
  EXPECT_TRUE(eitherPtr->error());
  EXPECT_EQ(*eitherPtr->error(), "propagated after async");
  delete eitherPtr;
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
