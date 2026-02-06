// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestAwaiters.hpp"
#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)

// =============================================================================
// Helper Coroutines for Leak Detection Tests
// =============================================================================

namespace
{
auto singleError(int code, std::string msg) -> Either<int, LeakDetectorError>
{
  co_return LeakDetectorError{code, std::move(msg)};
}

auto singleValue(int val) -> Either<int, LeakDetectorError> { co_return val; }

auto innerReturnsError() -> Either<int, LeakDetectorError>
{
  co_return LeakDetectorError{1, "inner error"};
}

auto middlePropagatesError() -> Either<int, LeakDetectorError>
{
  int val = co_await innerReturnsError();
  co_return val + 1;
}

auto outerReceivesError() -> Either<int, LeakDetectorError>
{
  int val = co_await middlePropagatesError();
  co_return val + 1;
}

// Deep nesting: 5 levels (prefixed to avoid conflict with TestHelpers.hpp)
auto leakLevel5Error() -> Either<int, LeakDetectorError>
{
  co_return LeakDetectorError{5, "level5 error"};
}

auto leakLevel4Error() -> Either<int, LeakDetectorError>
{
  int v = co_await leakLevel5Error();
  co_return v + 1;
}

auto leakLevel3Error() -> Either<int, LeakDetectorError>
{
  int v = co_await leakLevel4Error();
  co_return v + 1;
}

auto leakLevel2Error() -> Either<int, LeakDetectorError>
{
  int v = co_await leakLevel3Error();
  co_return v + 1;
}

auto leakLevel1Error() -> Either<int, LeakDetectorError>
{
  int v = co_await leakLevel2Error();
  co_return v + 1;
}

// Multiple co_await with some errors
auto firstOfTwo() -> Either<int, LeakDetectorError> { co_return 10; }

auto secondOfTwoError() -> Either<int, LeakDetectorError>
{
  co_return LeakDetectorError{2, "second error"};
}

auto multipleCoAwait() -> Either<int, LeakDetectorError>
{
  int a = co_await firstOfTwo();
  int b = co_await secondOfTwoError(); // This should propagate error
  co_return a + b;
}

// Conditional error based on parameter
auto conditionalError(bool shouldFail) -> Either<int, LeakDetectorError>
{
  if (shouldFail)
    co_return LeakDetectorError{99, "conditional error"};
  co_return 42;
}

auto chainConditional(bool innerFail, bool outerFail)
    -> Either<int, LeakDetectorError>
{
  int val = co_await conditionalError(innerFail);
  if (outerFail)
    co_return LeakDetectorError{100, "outer error after success"};
  co_return val * 2;
}
} // namespace

// =============================================================================
// Test Cases
// =============================================================================

TEST(M1S11_EitherErrorOwnershipLeak, U01_SingleErrorNoLeak)
{
  RecordProperty("id", "M1-S11-U01");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc", "Single coroutine returning error should not leak memory");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  {
    auto result = singleError(404, "not found");
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    EXPECT_EQ(result.error()->code, 404);
    EXPECT_EQ(result.error()->message, "not found");
  }

  // After Either destroyed, instance count should be 0
  EXPECT_EQ(LeakDetectorError::count(), 0)
      << "Memory leak detected: "
      << LeakDetectorError::count()
      << " instance(s) not destroyed";
}

TEST(M1S11_EitherErrorOwnershipLeak, U02_NestedErrorPropagationNoLeak)
{
  RecordProperty("id", "M1-S11-U02");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc",
      "Error propagated through nested coroutines should not leak memory");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  {
    auto result = outerReceivesError();
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    EXPECT_EQ(result.error()->code, 1);
    EXPECT_EQ(result.error()->message, "inner error");
  }

  EXPECT_EQ(LeakDetectorError::count(), 0)
      << "Memory leak detected: "
      << LeakDetectorError::count()
      << " instance(s) not destroyed";
}

TEST(M1S11_EitherErrorOwnershipLeak, U03_DeepNestingErrorNoLeak)
{
  RecordProperty("id", "M1-S11-U03");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc",
      "Error propagated through 5-level nested coroutines should not leak");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  {
    auto result = leakLevel1Error();
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    EXPECT_EQ(result.error()->code, 5);
    EXPECT_EQ(result.error()->message, "level5 error");
  }

  EXPECT_EQ(LeakDetectorError::count(), 0)
      << "Memory leak detected: "
      << LeakDetectorError::count()
      << " instance(s) not destroyed";
}

TEST(M1S11_EitherErrorOwnershipLeak, U04_MultipleCoAwaitWithErrors)
{
  RecordProperty("id", "M1-S11-U04");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc", "Multiple co_await where second returns error should not leak");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  {
    auto result = multipleCoAwait();
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    EXPECT_EQ(result.error()->code, 2);
    EXPECT_EQ(result.error()->message, "second error");
  }

  EXPECT_EQ(LeakDetectorError::count(), 0)
      << "Memory leak detected: "
      << LeakDetectorError::count()
      << " instance(s) not destroyed";
}

TEST(M1S11_EitherErrorOwnershipLeak, U05_InnerSuccessOuterError)
{
  RecordProperty("id", "M1-S11-U05");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc",
      "Inner coroutine succeeds, outer returns error - should not leak");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  {
    // innerFail=false, outerFail=true
    auto result = chainConditional(false, true);
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    EXPECT_EQ(result.error()->code, 100);
    EXPECT_EQ(result.error()->message, "outer error after success");
  }

  EXPECT_EQ(LeakDetectorError::count(), 0)
      << "Memory leak detected: "
      << LeakDetectorError::count()
      << " instance(s) not destroyed";
}

TEST(M1S11_EitherErrorOwnershipLeak, U06_InnerErrorIgnoresOuterError)
{
  RecordProperty("id", "M1-S11-U06");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc",
      "When inner fails, outer error code never executes - verify no leak");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  {
    // innerFail=true, outerFail=true (but outer error never created)
    auto result = chainConditional(true, true);
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    // Should get inner error, not outer
    EXPECT_EQ(result.error()->code, 99);
    EXPECT_EQ(result.error()->message, "conditional error");
  }

  EXPECT_EQ(LeakDetectorError::count(), 0)
      << "Memory leak detected: "
      << LeakDetectorError::count()
      << " instance(s) not destroyed";
}

TEST(M1S11_EitherErrorOwnershipLeak, U07_ValuePathNoErrorCreated)
{
  RecordProperty("id", "M1-S11-U07");
  RecordProperty("ver", "0.04");
  RecordProperty("desc", "Success path should never allocate error");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  {
    auto result = singleValue(42);
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.value());
    ASSERT_FALSE(result.error());
    EXPECT_EQ(*result.value(), 42);
  }

  // No error was ever created
  EXPECT_EQ(LeakDetectorError::count(), 0);
}

TEST(M1S11_EitherErrorOwnershipLeak, U08_ChainedSuccessNoLeak)
{
  RecordProperty("id", "M1-S11-U08");
  RecordProperty("ver", "0.04");
  RecordProperty("desc", "Chained success coroutines should not leak");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  {
    // Both inner and outer succeed
    auto result = chainConditional(false, false);
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.value());
    ASSERT_FALSE(result.error());
    EXPECT_EQ(*result.value(), 84); // 42 * 2
  }

  EXPECT_EQ(LeakDetectorError::count(), 0);
}

// =============================================================================
// Async Leak Tests
// =============================================================================

namespace
{
auto asyncInnerError(std::shared_ptr<std::mutex> mutex)
    -> Either<int, LeakDetectorError>
{
  co_await AsyncDelayAwaiter{std::chrono::milliseconds(10), mutex};
  co_return LeakDetectorError{999, "async inner error"};
}

auto asyncOuterPropagates(std::shared_ptr<std::mutex> mutex)
    -> Either<int, LeakDetectorError>
{
  int val = co_await asyncInnerError(mutex);
  co_return val + 1;
}
} // namespace

TEST(M1S11_EitherErrorOwnershipLeak, U09_AsyncErrorPropagationNoLeak)
{
  RecordProperty("id", "M1-S11-U09");
  RecordProperty("ver", "0.04");
  RecordProperty(
      "desc", "Async error propagation through co_await should not leak");

  LeakDetectorError::reset();
  ASSERT_EQ(LeakDetectorError::count(), 0);

  auto mutex = std::make_shared<std::mutex>();

  {
    auto result = asyncOuterPropagates(mutex);

    // Poll until done
    ASSERT_TRUE(pollUntilDone(
        [&result, &mutex]
        {
          std::lock_guard lock{*mutex};
          return result.done();
        },
        std::chrono::seconds(2)));

    ASSERT_TRUE(result.error());
    EXPECT_EQ(result.error()->code, 999);
    EXPECT_EQ(result.error()->message, "async inner error");
  }

  EXPECT_EQ(LeakDetectorError::count(), 0)
      << "Memory leak detected: "
      << LeakDetectorError::count()
      << " instance(s) not destroyed";
}

// NOLINTEND(readability-magic-numbers)
