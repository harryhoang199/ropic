// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include "TestHelpers.hpp"

TEST(EitherCoroutine, UNIT_013_BasicCoreturn)
{
  RecordProperty("id", "0.01-UNIT-013");
  RecordProperty("desc", "Coroutine co_return works for data, error, and Void");

  auto dataResult = returnData(42);
  ASSERT_TRUE(dataResult.data());
  EXPECT_EQ(*dataResult.data(), 42);

  auto errorResult = returnError("coroutine error");
  ASSERT_TRUE(errorResult.error());
  EXPECT_EQ(*errorResult.error(), "coroutine error");

  auto voidResult = returnOK();
  EXPECT_FALSE(voidResult.error());
}

TEST(EitherCoroutine, UNIT_014_ResultAccessible)
{
  RecordProperty("id", "0.01-UNIT-014");
  RecordProperty("desc", "Result accessible after coroutine completes (final_suspend works)");

  auto result = chainedAwaitsAllSucceed(0);
  ASSERT_TRUE(result.data());
  EXPECT_EQ(*result.data(), 110);
}

TEST(EitherCoroutine, UNIT_015_DestructorCleanup)
{
  RecordProperty("id", "0.01-UNIT-015");
  RecordProperty("desc", "Destructor handles cleanup when result not accessed");

  for (int i = 0; i < 100; ++i)
  {
    auto result = returnData(i);
    (void)result;
  }
  SUCCEED();
}

TEST(EitherCoroutine, UNIT_016_MoveOperations)
{
  RecordProperty("id", "0.01-UNIT-016");
  RecordProperty("desc", "Move from coroutine Either works, original destructor safe");

  auto src = returnData(42);
  auto dst = std::move(src);
  ASSERT_TRUE(dst.data());
  EXPECT_EQ(*dst.data(), 42);
}

TEST(EitherCoroutine, UNIT_017_ZeroCopiesOnReturn)
{
  RecordProperty("id", "0.01-UNIT-017");
  RecordProperty("desc", "Coroutine return uses move semantics");

  MoveTracker::reset();
  auto result = returnMoveTracker(42);
  ASSERT_TRUE(result.data());
  EXPECT_EQ(result.data()->value, 42);
  EXPECT_EQ(MoveTracker::copyCount, 0);
}

TEST(EitherCoroutine, UNIT_018_CoawaitBehavior)
{
  RecordProperty("id", "0.01-UNIT-018");
  RecordProperty("desc", "co_await continues on data, stops and propagates on error");

  auto successResult = awaitAndAdd(returnData(10), 5);
  ASSERT_TRUE(successResult.data());
  EXPECT_EQ(*successResult.data(), 15);

  auto errorResult = awaitAndAdd(returnError("input error"), 5);
  ASSERT_TRUE(errorResult.error());
  EXPECT_EQ(*errorResult.error(), "input error");
}

TEST(EitherCoroutine, UNIT_019_ChainedCoawait)
{
  RecordProperty("id", "0.01-UNIT-019");
  RecordProperty("desc", "Chained co_await stops at first error");

  auto allSucceed = chainedAwaitsAllSucceed(1);
  ASSERT_TRUE(allSucceed.data());
  EXPECT_EQ(*allSucceed.data(), 111);

  auto firstFails = chainedAwaitsFirstFails();
  ASSERT_TRUE(firstFails.error());
  EXPECT_EQ(*firstFails.error(), "first failed");

  auto middleFails = chainedAwaitsMiddleFails(1);
  ASSERT_TRUE(middleFails.error());
  EXPECT_EQ(*middleFails.error(), "middle failed");
}

TEST(EitherCoroutine, UNIT_020_CoawaitZeroCopies)
{
  RecordProperty("id", "0.01-UNIT-020");
  RecordProperty("desc", "co_await moves data/error, no copies");

  MoveTracker::reset();
  auto result = awaitMoveTracker(32);
  ASSERT_TRUE(result.data());
  EXPECT_EQ(result.data()->value, 42);
  EXPECT_EQ(MoveTracker::copyCount, 0);

  MoveTracker::reset();
  auto errResult = returnIntWithMoveTrackerError(true);
  ASSERT_TRUE(errResult.error());
  EXPECT_EQ(MoveTracker::copyCount, 0);
}

TEST(EitherCoroutine, UNIT_021_NestedCoroutines)
{
  RecordProperty("id", "0.01-UNIT-021");
  RecordProperty("desc", "Nested coroutines propagate data and errors correctly");

  auto success = outerCallsInnerSuccess(10);
  ASSERT_TRUE(success.data());
  EXPECT_EQ(*success.data(), 25);

  auto error = outerCallsInnerError();
  ASSERT_TRUE(error.error());
  EXPECT_EQ(*error.error(), "inner error");
}

TEST(EitherCoroutine, UNIT_022_MixedTypes)
{
  RecordProperty("id", "0.01-UNIT-022");
  RecordProperty("desc", "co_await Either<A, Err> in Either<B, Err> coroutine");

  auto result = mixedTypeCoroutine(10);
  ASSERT_TRUE(result.data());
  EXPECT_DOUBLE_EQ(*result.data(), 15.0);
}

TEST(EitherCoroutine, UNIT_023_VoidValidation)
{
  RecordProperty("id", "0.01-UNIT-023");
  RecordProperty("desc", "co_await Either<Void, Err> for validation");

  auto success = computeWithValidation(5);
  ASSERT_TRUE(success.data());
  EXPECT_EQ(*success.data(), 10);

  auto failure = computeWithValidation(-1);
  ASSERT_TRUE(failure.error());
  EXPECT_EQ(*failure.error(), "must be positive");
}

TEST(EitherCoroutine, UNIT_024_DeepNesting)
{
  RecordProperty("id", "0.01-UNIT-024");
  RecordProperty("desc", "Deep coroutine nesting works for success and error");

  auto deepSuccess = level1(0);
  ASSERT_TRUE(deepSuccess.data());
  EXPECT_EQ(*deepSuccess.data(), 5);

  auto deepError = level1Error();
  ASSERT_TRUE(deepError.error());
  EXPECT_EQ(*deepError.error(), "deep error");
}
