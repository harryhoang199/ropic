// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include "TestHelpers.hpp"
#include <chrono>
#include <iostream>
#include <utility>

// =============================================================================
// Recursive Function Implementations
// =============================================================================

/**
 * @brief Recursive function using co_await for error propagation.
 *
 * @param depth Controls the recursion depth (decrements toward 0)
 * @param errorAt Specifies the depth at which an error occurs (decrements toward 0)
 * @return Either<int, std::string> Success with depth value, or error string
 *
 * Termination:
 * - depth == 0: Return success with depth value
 * - errorAt == 0: Return error with initial errorAt value in message
 */
Either<int, std::string> recursiveCoawait(int depth, int errorAt) noexcept
{
  if (errorAt == 0)
  {
    co_return std::string("Error at depth " + std::to_string(depth));
  }
  if (depth == 0)
  {
    co_return depth;
  }
  int result = co_await recursiveCoawait(depth - 1, errorAt - 1);
  co_return result;
}

/**
 * @brief Recursive function using throw for error propagation.
 *
 * @param depth Controls the recursion depth (decrements toward 0)
 * @param errorAt Specifies the depth at which an error occurs (decrements toward 0)
 * @return int Success with depth value
 * @throws std::string Error message when errorAt reaches 0
 *
 * Termination:
 * - depth == 0: Return success with depth value
 * - errorAt == 0: Throw error with initial errorAt value in message
 */
int recursiveThrow(int depth, int errorAt)
{
  if (errorAt == 0)
  {
    throw std::string("Error at depth " + std::to_string(depth));
  }
  if (depth == 0)
  {
    return depth;
  }
  return recursiveThrow(depth - 1, errorAt - 1);
}

/**
 * @brief Recursive function using explicit checks for error propagation.
 *
 * @param depth Controls the recursion depth (decrements toward 0)
 * @param errorAt Specifies the depth at which an error occurs (decrements toward 0)
 * @return std::pair<int, std::string> Success: {value, ""}, Error: {-1, errorMessage}
 *
 * Termination:
 * - depth == 0: Return success tuple {depth, ""}
 * - errorAt == 0: Return error tuple {-1, errorMessage}
 */
std::pair<int, std::string> recursiveExplicit(int depth, int errorAt) noexcept
{
  if (errorAt == 0)
  {
    return {-1, "Error at depth " + std::to_string(depth)};
  }
  if (depth == 0)
  {
    return {depth, ""};
  }
  auto result = recursiveExplicit(depth - 1, errorAt - 1);
  if (result.first < 0)
  {
    return {-1, result.second};
  }
  return result;
}

// =============================================================================
// Driver Function with Timing
// =============================================================================

/**
 * @brief Driver function that executes and benchmarks all three recursive functions.
 *
 * @param depth Recursion depth for all functions
 * @param errorAt Error trigger depth for all functions
 *
 * Measures execution time for each function and prints results to stdout.
 * Verifies that all functions produce consistent results (same success/error behavior).
 */
void benchmarkDriver(int depth, int errorAt)
{
  using Clock = std::chrono::high_resolution_clock;
  using Duration = std::chrono::duration<double, std::micro>;

  std::cout << "\n=== Benchmark: depth=" << depth << ", errorAt=" << errorAt << " ===" << std::endl;

  // --- co_await version ---
  auto startCoawait = Clock::now();
  auto resultCoawait = recursiveCoawait(depth, errorAt);
  auto endCoawait = Clock::now();
  Duration durationCoawait = endCoawait - startCoawait;

  bool coawaitHasError = static_cast<bool>(resultCoawait.error());
  int coawaitValue = coawaitHasError ? -1 : *resultCoawait.data();
  std::string coawaitError = coawaitHasError ? *resultCoawait.error() : "";

  std::cout << "  co_await:  " << durationCoawait.count() << " us"
            << (coawaitHasError ? " [ERROR: " + coawaitError + "]" : " [OK: " + std::to_string(coawaitValue) + "]")
            << std::endl;

  // --- throw version ---
  auto startThrow = Clock::now();
  bool throwHasError = false;
  int throwValue = -1;
  std::string throwError;
  try
  {
    throwValue = recursiveThrow(depth, errorAt);
  }
  catch (const std::string &e)
  {
    throwHasError = true;
    throwError = e;
  }
  auto endThrow = Clock::now();
  Duration durationThrow = endThrow - startThrow;

  std::cout << "  throw:     " << durationThrow.count() << " us"
            << (throwHasError ? " [ERROR: " + throwError + "]" : " [OK: " + std::to_string(throwValue) + "]")
            << std::endl;

  // --- explicit check version ---
  auto startExplicit = Clock::now();
  auto resultExplicit = recursiveExplicit(depth, errorAt);
  auto endExplicit = Clock::now();
  Duration durationExplicit = endExplicit - startExplicit;

  bool explicitHasError = resultExplicit.first < 0;
  int explicitValue = resultExplicit.first;
  std::string explicitError = resultExplicit.second;

  std::cout << "  explicit:  " << durationExplicit.count() << " us"
            << (explicitHasError ? " [ERROR: " + explicitError + "]" : " [OK: " + std::to_string(explicitValue) + "]")
            << std::endl;

  // Verify consistency across all implementations
  EXPECT_EQ(coawaitHasError, throwHasError) << "co_await and throw disagree on error state";
  EXPECT_EQ(coawaitHasError, explicitHasError) << "co_await and explicit disagree on error state";

  if (!coawaitHasError)
  {
    EXPECT_EQ(coawaitValue, throwValue) << "co_await and throw return different values";
    EXPECT_EQ(coawaitValue, explicitValue) << "co_await and explicit return different values";
  }
}

// =============================================================================
// Test Cases - Boundary Values
// =============================================================================

TEST(EitherRecursiveBenchmark, BENCH_001_ZeroDepthSuccess)
{
  RecordProperty("id", "0.01-BENCH-001");
  RecordProperty("desc", "Zero depth returns success immediately");

  // depth=0, errorAt=1: Should return success (depth reaches 0 first)
  benchmarkDriver(0, 1);

  auto result = recursiveCoawait(0, 1);
  ASSERT_FALSE(result.error());
  EXPECT_EQ(*result.data(), 0);
}

TEST(EitherRecursiveBenchmark, BENCH_002_ZeroErrorAtFailure)
{
  RecordProperty("id", "0.01-BENCH-002");
  RecordProperty("desc", "Zero errorAt returns error immediately");

  // depth=1, errorAt=0: Should return error (errorAt reaches 0 first)
  benchmarkDriver(1, 0);

  auto result = recursiveCoawait(1, 0);
  ASSERT_TRUE(result.error());
  EXPECT_NE(result.error()->find("Error"), std::string::npos);
}

TEST(EitherRecursiveBenchmark, BENCH_003_BothZeroBoundary)
{
  RecordProperty("id", "0.01-BENCH-003");
  RecordProperty("desc", "Both parameters zero - errorAt checked first");

  // depth=0, errorAt=0: errorAt is checked first, so should return error
  benchmarkDriver(0, 0);

  auto result = recursiveCoawait(0, 0);
  ASSERT_TRUE(result.error());
}

TEST(EitherRecursiveBenchmark, BENCH_004_DepthEqualsErrorAt)
{
  RecordProperty("id", "0.01-BENCH-004");
  RecordProperty("desc", "Depth equals errorAt - errorAt checked first, returns error");

  // depth=5, errorAt=5: Both decrement together, both reach 0, errorAt checked first → error
  benchmarkDriver(5, 5);

  auto result = recursiveCoawait(5, 5);
  ASSERT_TRUE(result.error());
}

TEST(EitherRecursiveBenchmark, BENCH_004b_DepthOneLessThanErrorAt)
{
  RecordProperty("id", "0.01-BENCH-004b");
  RecordProperty("desc", "Depth one less than errorAt - depth reaches 0 first, success");

  // depth=5, errorAt=6: depth reaches 0 when errorAt is still 1 → success
  benchmarkDriver(5, 6);

  auto result = recursiveCoawait(5, 6);
  ASSERT_FALSE(result.error());
  EXPECT_EQ(*result.data(), 0);
}

TEST(EitherRecursiveBenchmark, BENCH_005_ErrorBeforeSuccess)
{
  RecordProperty("id", "0.01-BENCH-005");
  RecordProperty("desc", "Error occurs before reaching success depth");

  // depth=10, errorAt=3: Error at depth 7 (10-3)
  benchmarkDriver(10, 3);

  auto result = recursiveCoawait(10, 3);
  ASSERT_TRUE(result.error());
}

TEST(EitherRecursiveBenchmark, BENCH_006_SuccessBeforeError)
{
  RecordProperty("id", "0.01-BENCH-006");
  RecordProperty("desc", "Success reached before error trigger");

  // depth=3, errorAt=10: Success reached at depth 0
  benchmarkDriver(3, 10);

  auto result = recursiveCoawait(3, 10);
  ASSERT_FALSE(result.error());
  EXPECT_EQ(*result.data(), 0);
}

// =============================================================================
// Test Cases - Stress Testing with Large Values
// =============================================================================

TEST(EitherRecursiveBenchmark, BENCH_010_ModerateDepthSuccess)
{
  RecordProperty("id", "0.01-BENCH-010");
  RecordProperty("desc", "Moderate depth (100) with success path");

  benchmarkDriver(100, 200);

  auto result = recursiveCoawait(100, 200);
  ASSERT_FALSE(result.error());
  EXPECT_EQ(*result.data(), 0);
}

TEST(EitherRecursiveBenchmark, BENCH_011_ModerateDepthError)
{
  RecordProperty("id", "0.01-BENCH-011");
  RecordProperty("desc", "Moderate depth (100) with error path");

  benchmarkDriver(100, 50);

  auto result = recursiveCoawait(100, 50);
  ASSERT_TRUE(result.error());
}

TEST(EitherRecursiveBenchmark, BENCH_012_LargeDepthSuccess)
{
  RecordProperty("id", "0.01-BENCH-012");
  RecordProperty("desc", "Large depth (200) with success path");

  benchmarkDriver(200, 400);

  auto result = recursiveCoawait(200, 400);
  ASSERT_FALSE(result.error());
  EXPECT_EQ(*result.data(), 0);
}

TEST(EitherRecursiveBenchmark, BENCH_013_LargeDepthError)
{
  RecordProperty("id", "0.01-BENCH-013");
  RecordProperty("desc", "Large depth (200) with error path");

  benchmarkDriver(200, 100);

  auto result = recursiveCoawait(200, 100);
  ASSERT_TRUE(result.error());
}

// Note: Recursive coroutine depth is limited by stack size.
// These stress tests use moderate depths to avoid stack overflow.

TEST(EitherRecursiveBenchmark, BENCH_014_StressDepthSuccess)
{
  RecordProperty("id", "0.01-BENCH-014");
  RecordProperty("desc", "Stress depth (300) with success path - compares performance");

  benchmarkDriver(300, 600);

  auto result = recursiveCoawait(300, 600);
  ASSERT_FALSE(result.error());
  EXPECT_EQ(*result.data(), 0);
}

TEST(EitherRecursiveBenchmark, BENCH_015_StressDepthEarlyError)
{
  RecordProperty("id", "0.01-BENCH-015");
  RecordProperty("desc", "Stress depth (300) with early error");

  // Error occurs after just 10 levels of recursion
  benchmarkDriver(300, 10);

  auto result = recursiveCoawait(300, 10);
  ASSERT_TRUE(result.error());
}

TEST(EitherRecursiveBenchmark, BENCH_016_StressDepthLateError)
{
  RecordProperty("id", "0.01-BENCH-016");
  RecordProperty("desc", "Stress depth (300) with late error");

  // Error occurs just before reaching success
  benchmarkDriver(300, 299);

  auto result = recursiveCoawait(300, 299);
  ASSERT_TRUE(result.error());
}

// =============================================================================
// Test Cases - Edge Cases
// =============================================================================

TEST(EitherRecursiveBenchmark, BENCH_020_SingleRecursion)
{
  RecordProperty("id", "0.01-BENCH-020");
  RecordProperty("desc", "Single recursion level");

  // depth=1, errorAt=2: One level of recursion, then success
  benchmarkDriver(1, 2);

  auto result = recursiveCoawait(1, 2);
  ASSERT_FALSE(result.error());
  EXPECT_EQ(*result.data(), 0);
}

TEST(EitherRecursiveBenchmark, BENCH_021_ErrorAtLastMoment)
{
  RecordProperty("id", "0.01-BENCH-021");
  RecordProperty("desc", "Error triggers at the last possible moment");

  // depth=5, errorAt=1: Goes through 4 levels, then errors on 5th
  benchmarkDriver(5, 1);

  auto result = recursiveCoawait(5, 1);
  ASSERT_TRUE(result.error());
}

TEST(EitherRecursiveBenchmark, BENCH_022_LargeErrorAtValue)
{
  RecordProperty("id", "0.01-BENCH-022");
  RecordProperty("desc", "Very large errorAt value (never triggers)");

  // depth=10, errorAt=INT_MAX/2: errorAt never reaches 0
  benchmarkDriver(10, 10000);

  auto result = recursiveCoawait(10, 10000);
  ASSERT_FALSE(result.error());
  EXPECT_EQ(*result.data(), 0);
}
