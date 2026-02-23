// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)

TEST(M1S05EitherSyncCoroutine, U01BasicCoreturn)
{
  RecordProperty("id", "M1-S05-U01");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Coroutine co_return works for data, error, and Void");

  auto dataResult = returnData(42);
  ASSERT_EQ(dataResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(dataResult.value());
  EXPECT_EQ(*dataResult.value(), 42);

  auto errorResult = returnError("coroutine error");
  ASSERT_EQ(errorResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(errorResult.error());
  EXPECT_EQ(*errorResult.error(), "coroutine error");

  auto voidResult = returnOK();
  ASSERT_EQ(voidResult.state(), ropic::CoroState::DONE);
  EXPECT_FALSE(voidResult.error());
}

TEST(M1S05EitherSyncCoroutine, U02ResultAccessible)
{
  RecordProperty("id", "M1-S05-U02");
  RecordProperty("ver", "0.01");
  RecordProperty(
      "desc",
      "Result accessible after coroutine completes "
      "(final_suspend works)");

  auto chainedAwaitsAllSucceed = [](int start) -> Either<int, std::string>
  {
    int a = co_await returnData(start);
    int b = co_await returnData(a + 10);
    int c = co_await returnData(b + 100);
    co_return c;
  };

  auto result = chainedAwaitsAllSucceed(0);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 110);
}

TEST(M1S05EitherSyncCoroutine, U03DestructorCleanup)
{
  RecordProperty("id", "M1-S05-U03");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Destructor handles cleanup when result not accessed");

  for (int i = 0; i < 100; ++i)
  {
    auto result = returnData(i);
    (void)result;
  }
  SUCCEED();
}

TEST(M1S05EitherSyncCoroutine, U04MoveOperations)
{
  RecordProperty("id", "M1-S05-U04");
  RecordProperty("ver", "0.01");
  RecordProperty(
      "desc", "Move from coroutine Either works, original destructor safe");

  auto src = returnData(42);
  auto dst = std::move(src);
  ASSERT_EQ(dst.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(dst.value());
  EXPECT_EQ(*dst.value(), 42);
}

TEST(M1S05EitherSyncCoroutine, U05ZeroCopiesOnReturn)
{
  RecordProperty("id", "M1-S05-U05");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Coroutine return uses move semantics");

  using MT = MoveTracker<"M1-S05-U05">;

  MT::reset();
  auto result = returnMoveTracker<"M1-S05-U05">(42);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(result.value()->value, 42);
  EXPECT_EQ(MT::s_copyCount, 0);
}

TEST(M1S05EitherSyncCoroutine, U06CoawaitBehavior)
{
  RecordProperty("id", "M1-S05-U06");
  RecordProperty("ver", "0.01");
  RecordProperty(
      "desc", "co_await continues on data, stops and propagates on error");

  {
    auto successResult = awaitAndAdd(returnData(10), 5);
    ASSERT_EQ(successResult.state(), ropic::CoroState::DONE);
    ASSERT_TRUE(successResult.value());
    EXPECT_EQ(*successResult.value(), 15);
  }
  auto errorResult = awaitAndAdd(returnError("input error"), 5);
  ASSERT_EQ(errorResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(errorResult.error());
  EXPECT_EQ(*errorResult.error(), "input error");
}

TEST(M1S05EitherSyncCoroutine, U07CoawaitRvalueAndLvalue)
{
  RecordProperty("id", "M1-S05-U07");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "co_await works on both rvalue and lvalue Either");

  auto rvalueResult = awaitAndAdd(returnData(10), 5);
  ASSERT_EQ(rvalueResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(rvalueResult.value());
  EXPECT_EQ(*rvalueResult.value(), 15);

  auto input = returnData(20);
  auto lvalueResult = awaitAndAdd(std::move(input), 5);
  ASSERT_EQ(lvalueResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(lvalueResult.value());
  EXPECT_EQ(*lvalueResult.value(), 25);
}

TEST(M1S05EitherSyncCoroutine, U08ChainedCoawait)
{
  RecordProperty("id", "M1-S05-U08");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Chained co_await stops at first error");

  auto chainedAwaitsAllSucceed = [](int start) -> Either<int, std::string>
  {
    int a = co_await returnData(start);
    int b = co_await returnData(a + 10);
    int c = co_await returnData(b + 100);
    co_return c;
  };

  auto chainedAwaitsFirstFails = []() -> Either<int, std::string>
  {
    int a = co_await returnError("first failed");
    int b = co_await returnData(a + 10);
    co_return b;
  };

  auto chainedAwaitsMiddleFails = [](int start) -> Either<int, std::string>
  {
    [[maybe_unused]]
    int a = co_await returnData(start);
    int b = co_await returnError("middle failed");
    co_return b + 100;
  };

  auto allSucceed = chainedAwaitsAllSucceed(1);
  ASSERT_EQ(allSucceed.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(allSucceed.value());
  EXPECT_EQ(*allSucceed.value(), 111);

  auto firstFails = chainedAwaitsFirstFails();
  ASSERT_EQ(firstFails.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(firstFails.error());
  EXPECT_EQ(*firstFails.error(), "first failed");

  auto middleFails = chainedAwaitsMiddleFails(1);
  ASSERT_EQ(middleFails.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(middleFails.error());
  EXPECT_EQ(*middleFails.error(), "middle failed");
}

TEST(M1S05EitherSyncCoroutine, U09CoawaitZeroCopies)
{
  RecordProperty("id", "M1-S05-U09");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "co_await moves data/error, no copies");

  using MT = MoveTracker<"M1-S05-U09">;

  auto awaitMoveTracker = [](int x) -> Either<MT, std::string>
  {
    MT val = co_await returnMoveTracker<"M1-S05-U09">(x);
    co_return MT{val.value + 10};
  };

  MT::reset();
  auto result = awaitMoveTracker(32);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(result.value()->value, 42);
  EXPECT_EQ(MT::s_copyCount, 0);

  MT::reset();
  auto errResult = returnIntWithMoveTrackerError<"M1-S05-U09">(true);
  ASSERT_EQ(errResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(errResult.error());
  EXPECT_EQ(MT::s_copyCount, 0);
}

TEST(M1S05EitherSyncCoroutine, U10NestedCoroutines)
{
  RecordProperty("id", "M1-S05-U10");
  RecordProperty("ver", "0.01");
  RecordProperty(
      "desc", "Nested coroutines propagate data and errors correctly");

  auto innerSuccess = [](int x) -> Either<int, std::string>
  { co_return x * 2; };

  auto innerError = []() -> Either<int, std::string>
  { co_return std::string("inner error"); };

  auto outerCallsInnerSuccess =
      [&innerSuccess](int x) -> Either<int, std::string>
  {
    int result = co_await innerSuccess(x);
    co_return result + 5;
  };

  auto outerCallsInnerError = [&innerError]() -> Either<int, std::string>
  {
    int result = co_await innerError();
    co_return result + 5;
  };

  auto success = outerCallsInnerSuccess(10);
  ASSERT_EQ(success.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(success.value());
  EXPECT_EQ(*success.value(), 25);

  auto error = outerCallsInnerError();
  ASSERT_EQ(error.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(error.error());
  EXPECT_EQ(*error.error(), "inner error");
}

TEST(M1S05EitherSyncCoroutine, U11MixedTypes)
{
  RecordProperty("id", "M1-S05-U11");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "co_await Either<A, Err> in Either<B, Err> coroutine");

  auto mixedTypeCoroutine = [](int x) -> Either<double, std::string>
  {
    int val = co_await returnData(x);
    co_return static_cast<double>(val) * 1.5;
  };

  auto result = mixedTypeCoroutine(10);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_DOUBLE_EQ(*result.value(), 15.0);
}

TEST(M1S05EitherSyncCoroutine, U12VoidValidation)
{
  RecordProperty("id", "M1-S05-U12");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "co_await Either<Void, Err> for validation");

  auto validatePositive = [](int x) -> Either<Void, std::string>
  {
    if (x <= 0)
      co_return std::string("must be positive");
    co_return OK;
  };

  auto computeWithValidation =
      [&validatePositive](int x) -> Either<int, std::string>
  {
    co_await validatePositive(x);
    co_return x * 2;
  };

  auto success = computeWithValidation(5);
  ASSERT_EQ(success.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(success.value());
  EXPECT_EQ(*success.value(), 10);

  auto failure = computeWithValidation(-1);
  ASSERT_EQ(failure.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(failure.error());
  EXPECT_EQ(*failure.error(), "must be positive");
}

TEST(M1S05EitherSyncCoroutine, U13DeepNesting)
{
  RecordProperty("id", "M1-S05-U13");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Deep coroutine nesting works for success and error");

  auto deepSuccess = level1(0);
  ASSERT_EQ(deepSuccess.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(deepSuccess.value());
  EXPECT_EQ(*deepSuccess.value(), 5);

  auto deepError = level1Error();
  ASSERT_EQ(deepError.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(deepError.error());
  EXPECT_EQ(*deepError.error(), "deep error");
}

TEST(M1S05EitherSyncCoroutine, U14CoawaitLvalueReferenceIdentity)
{
  RecordProperty("id", "M1-S05-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "co_await on lvalue Either yields reference to original data");

  auto lvalueEither = returnData(42);
  ASSERT_EQ(lvalueEither.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(lvalueEither.value());

  const int* addressFromValue = &(*lvalueEither.value());
  const int* addressFromCoawait = nullptr;

  auto testCoroutine = [&lvalueEither,
                        &addressFromCoawait]() -> Either<int, std::string>
  {
    auto& result = co_await lvalueEither;
    addressFromCoawait = &result;
    co_return result;
  };

  auto outer = testCoroutine();
  ASSERT_EQ(outer.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(outer.value());
  EXPECT_EQ(addressFromCoawait, addressFromValue);
}

// NOLINTEND(readability-magic-numbers)
