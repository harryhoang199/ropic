// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)

// =============================================================================
// Test Suite: Either-Coroutine Invoking Other Coroutines
// =============================================================================

TEST(M1S07EitherInteropToOther, U01EitherAwaitsCustomAwaiter)
{
  RecordProperty("id", "M1-S07-U01");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "Either coroutine co_awaits custom awaiter");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 42;
  };

  auto either = coro();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED);
  EXPECT_FALSE(either.value());
  EXPECT_FALSE(either.error());

  awaiter.resume();

  EXPECT_EQ(either.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(either.value());
  EXPECT_EQ(*either.value(), 42);
}

TEST(M1S07EitherInteropToOther, U02EitherAwaitsValueAwaiter)
{
  RecordProperty("id", "M1-S07-U02");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "Either coroutine receives value from custom awaiter");

  ManualResumeAwaiterWithValue<int> awaiter{100};

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    int value = co_await awaiter;
    co_return value + 5;
  };

  auto either = coro();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED);

  awaiter.resume();

  ASSERT_EQ(either.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(either.value());
  EXPECT_EQ(*either.value(), 105);
}

TEST(M1S07EitherInteropToOther, U03EitherAwaitsMultipleAwaiters)
{
  RecordProperty("id", "M1-S07-U03");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Either coroutine co_awaits multiple custom awaiters in "
      "sequence");

  ManualResumeAwaiterWithValue<int> awaiter1{10};
  ManualResumeAwaiterWithValue<int> awaiter2{20};

  auto coro = [&awaiter1, &awaiter2]() -> Either<int, std::string>
  {
    int v1 = co_await awaiter1;
    int v2 = co_await awaiter2;
    co_return v1 + v2;
  };

  auto either = coro();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED);

  awaiter1.resume();
  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED);

  awaiter2.resume();
  ASSERT_EQ(either.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(either.value());
  EXPECT_EQ(*either.value(), 30);
}

TEST(M1S07EitherInteropToOther, U04EitherAwaitsMixedAwaitables)
{
  RecordProperty("id", "M1-S07-U04");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Either coroutine mixes custom awaiters with Either error "
      "propagation");

  ManualResumeAwaiterWithValue<int> awaiter{50};

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    int v = co_await awaiter;
    int x = co_await returnData(v);
    co_return x + 10;
  };

  auto either = coro();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED);

  awaiter.resume();

  ASSERT_EQ(either.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(either.value());
  EXPECT_EQ(*either.value(), 60);
}

TEST(M1S07EitherInteropToOther, U05EitherAwaitsCustomThenPropagatesError)
{
  RecordProperty("id", "M1-S07-U05");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Either coroutine awaits custom awaiter then propagates "
      "Either error");

  ManualResumeAwaiterWithValue<int> awaiter{50};

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    [[maybe_unused]]
    int v = co_await awaiter;
    int x = co_await returnError("after custom");
    co_return x;
  };

  auto either = coro();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED);

  awaiter.resume();

  ASSERT_EQ(either.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(either.error());
  EXPECT_EQ(*either.error(), "after custom");
}

TEST(M1S07EitherInteropToOther, U06EitherAwaitsVoidAwaiter)
{
  RecordProperty("id", "M1-S07-U06");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Either<Void, Error> coroutine co_awaits custom awaiter");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<Void, std::string>
  {
    co_await awaiter;
    co_return OK;
  };

  auto either = coro();

  EXPECT_EQ(either.state(), ropic::CoroState::UNDEFINED);

  awaiter.resume();

  ASSERT_EQ(either.state(), ropic::CoroState::DONE);
  EXPECT_FALSE(either.error());
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
