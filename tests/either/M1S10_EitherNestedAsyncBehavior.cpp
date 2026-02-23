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

TEST(M1S10EitherNestedAsyncBehavior, U01SuspendedWithoutError)
{
  RecordProperty("id", "M1-S10-U01");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc",
      "Succeeded Either's done() returns false while coroutine is suspended");

  ManualResumeAwaiter awaiter1;
  ManualResumeAwaiter awaiter2;

  auto innermost = [&awaiter1]() -> Either<int, std::string>
  {
    co_await awaiter1;
    co_return 42;
  };

  auto middle = [&innermost, &awaiter2]() -> Either<int, std::string>
  {
    auto v = co_await innermost();
    co_await awaiter2;
    co_return v;
  };

  auto outermost = [&middle]() -> Either<int, std::string>
  { co_return co_await middle(); };

  auto either = outermost();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED)
      << "state() should return UNDEFINED while suspended";
  awaiter1.resume();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED)
      << "state() should return UNDEFINED while suspended";
  awaiter2.resume();

  EXPECT_EQ(either.state(), ropic::CoroState::DONE)
      << "done() should return true after resume";
  EXPECT_EQ(*either.value(), 42)
      << "value() should return valid Borrower after resume";
}

TEST(M1S10EitherNestedAsyncBehavior, U02SuspendedWithError)
{
  RecordProperty("id", "M1-S10-U02");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc",
      "Failed Either's done() returns false while coroutine is suspended");

  ManualResumeAwaiter awaiter1;
  ManualResumeAwaiter awaiter2;

  auto innermost = [&awaiter2]() -> Either<int, std::string>
  {
    co_await awaiter2;
    co_return std::string("ERROR 404");
  };

  auto middle = [&innermost, &awaiter1]() -> Either<int, std::string>
  {
    co_await awaiter1;
    auto v = co_await innermost();
    co_return v;
  };

  auto outermost = [&middle]() -> Either<int, std::string>
  { co_return co_await middle(); };

  auto either = outermost();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED)
      << "state() should return UNDEFINED while suspended";
  awaiter1.resume();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED)
      << "state() should return UNDEFINED while suspended";
  awaiter2.resume();

  EXPECT_EQ(either.state(), ropic::CoroState::DONE)
      << "done() should return true after resume";
  EXPECT_EQ(*either.error(), std::string("ERROR 404"))
      << "error() should return valid Borrower after resume";
}

TEST(M1S10EitherNestedAsyncBehavior, U03AsyncDelayDonePolling)
{
  RecordProperty("id", "M1-S10-U03");
  RecordProperty("ver", "0.04");
  RecordProperty("desc", "done() correctly reflects async completion state");

  auto mutex = std::make_shared<std::mutex>();

  auto innermost = [mutex]() -> Either<int, std::string>
  {
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(50), mutex};
    co_return 123;
  };

  auto middle = [&innermost, mutex]() -> Either<int, std::string>
  {
    auto v = co_await innermost();
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30), mutex};
    co_return v;
  };

  auto outermost = [&middle]() -> Either<int, std::string>
  { co_return co_await middle(); };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(outermost());
    EXPECT_EQ(eitherPtr->state(), ropic::CoroState::UNDEFINED)
        << "state() should return UNDEFINED immediately after creation";
  }

#ifdef NDEBUG
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";
#else
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      }));
#endif

  EXPECT_EQ(eitherPtr->state(), ropic::CoroState::DONE);
  EXPECT_TRUE(eitherPtr->value());
  EXPECT_EQ(*eitherPtr->value(), 123);
  delete eitherPtr;
}

TEST(M1S10EitherNestedAsyncBehavior, U04AsyncDelayWithError)
{
  RecordProperty("id", "M1-S10-U04");
  RecordProperty("ver", "0.04");
  RecordProperty("desc", "Async Either correctly reports error after delay");

  auto mutex = std::make_shared<std::mutex>();

  auto innermost = [mutex]() -> Either<int, std::string>
  {
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30), mutex};
    co_return std::string("async error");
  };

  auto middle = [&innermost, mutex]() -> Either<int, std::string>
  {
    auto v = co_await innermost();
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(50), mutex};
    co_return v;
  };

  auto outermost = [&middle]() -> Either<int, std::string>
  { co_return co_await middle(); };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(outermost());
    EXPECT_EQ(eitherPtr->state(), ropic::CoroState::UNDEFINED)
        << "state() should return UNDEFINED immediately after creation";
  }

#ifdef NDEBUG
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";
#else
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      }));
#endif

  EXPECT_EQ(eitherPtr->state(), ropic::CoroState::DONE);
  EXPECT_TRUE(eitherPtr->error());
  EXPECT_EQ(*eitherPtr->error(), "async error");
  delete eitherPtr;
}

TEST(M1S10EitherNestedAsyncBehavior, U05AsyncWithValueAwaiter)
{
  RecordProperty("id", "M1-S10-U05");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc", "Async Either receives value from async awaiter and computes");

  auto mutex = std::make_shared<std::mutex>();

  auto innermost = [mutex]() -> Either<int, std::string>
  {
    int value = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(20), 100, mutex};
    co_return value * 2;
  };

  auto middle = [&innermost, mutex]() -> Either<int, std::string>
  {
    auto v = co_await innermost();
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30), mutex};
    co_return v;
  };

  auto outermost = [&middle]() -> Either<int, std::string>
  { co_return co_await middle(); };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(outermost());
    EXPECT_EQ(eitherPtr->state(), ropic::CoroState::UNDEFINED)
        << "state() should return UNDEFINED immediately after creation";
  }

#ifdef NDEBUG
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";
#else
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      }));
#endif

  EXPECT_EQ(eitherPtr->state(), ropic::CoroState::DONE);
  EXPECT_TRUE(eitherPtr->value());
  EXPECT_EQ(*eitherPtr->value(), 200);
  delete eitherPtr;
}

TEST(M1S10EitherNestedAsyncBehavior, U06AsyncChainedOperations)
{
  RecordProperty("id", "M1-S10-U06");
  RecordProperty("ver", "0.04");
  RecordProperty("desc", "Async Either with multiple async awaiters in chain");

  auto mutex = std::make_shared<std::mutex>();

  auto innermost = [mutex]() -> Either<int, std::string>
  {
    int v1 = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(10), 10, mutex};
    int v2 = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(10), 20, mutex};
    co_return v1 + v2;
  };

  auto middle = [&innermost, mutex]() -> Either<int, std::string>
  {
    auto v = co_await innermost();
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30), mutex};
    co_return v;
  };

  auto outermost = [&middle]() -> Either<int, std::string>
  { co_return co_await middle(); };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(outermost());
    EXPECT_EQ(eitherPtr->state(), ropic::CoroState::UNDEFINED)
        << "state() should return UNDEFINED immediately after creation";
  }

#ifdef NDEBUG
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";
#else
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      }));
#endif

  EXPECT_EQ(eitherPtr->state(), ropic::CoroState::DONE);
  EXPECT_TRUE(eitherPtr->value());
  EXPECT_EQ(*eitherPtr->value(), 30);
  delete eitherPtr;
}

TEST(M1S10EitherNestedAsyncBehavior, U07AsyncMixedWithEitherPropagation)
{
  RecordProperty("id", "M1-S10-U07");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc",
      "Async Either mixes async awaiters with Either error propagation");

  auto mutex = std::make_shared<std::mutex>();

  auto innermost = [mutex]() -> Either<int, std::string>
  {
    int v = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(15), 50, mutex};
    int x = co_await returnData(v);
    co_return x + 5;
  };

  auto middle = [&innermost, mutex]() -> Either<int, std::string>
  {
    auto v = co_await innermost();
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30), mutex};
    co_return v;
  };

  auto outermost = [&middle]() -> Either<int, std::string>
  { co_return co_await middle(); };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(outermost());
    EXPECT_EQ(eitherPtr->state(), ropic::CoroState::UNDEFINED)
        << "state() should return UNDEFINED immediately after creation";
  }

#ifdef NDEBUG
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";
#else
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      }));
#endif

  EXPECT_EQ(eitherPtr->state(), ropic::CoroState::DONE);
  EXPECT_TRUE(eitherPtr->value());
  EXPECT_EQ(*eitherPtr->value(), 55);
  delete eitherPtr;
}

TEST(M1S10EitherNestedAsyncBehavior, U08AsyncErrorPropagationAfterDelay)
{
  RecordProperty("id", "M1-S10-U08");
  RecordProperty("ver", "0.04");
  RecordProperty("desc", "Async Either propagates error after async operation");

  auto mutex = std::make_shared<std::mutex>();

  auto innermost = [mutex]() -> Either<int, std::string>
  {
    [[maybe_unused]]
    int v = co_await AsyncValueAwaiter<int>{
        std::chrono::milliseconds(15), 50, mutex};
    int x = co_await returnError("propagated after async");
    co_return x;
  };

  auto middle = [&innermost, mutex]() -> Either<int, std::string>
  {
    auto v = co_await innermost();
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30), mutex};
    co_return v;
  };

  auto outermost = [&middle]() -> Either<int, std::string>
  { co_return co_await middle(); };

  Either<int, std::string>* eitherPtr;

  {
    std::lock_guard lock(*mutex);
    eitherPtr = new Either<int, std::string>(outermost());
    EXPECT_EQ(eitherPtr->state(), ropic::CoroState::UNDEFINED)
        << "state() should return UNDEFINED immediately after creation";
  }

#ifdef NDEBUG
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";
#else
  ASSERT_TRUE(pollUntilDone(
      [eitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return eitherPtr->state() == ropic::CoroState::DONE;
      }));
#endif

  EXPECT_EQ(eitherPtr->state(), ropic::CoroState::DONE);
  EXPECT_TRUE(eitherPtr->error());
  EXPECT_EQ(*eitherPtr->error(), "propagated after async");
  delete eitherPtr;
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
