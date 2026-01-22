// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

// =============================================================================
// Categories A-B: Workflow Benchmarks (Happy Path & Error Path)
// Compares: ropic::Either (co_await) vs try-catch (Throw) vs std::optional (IfElse)
// =============================================================================

#include <benchmark/benchmark.h>
#include "ComparativeHelpers.hpp"

using namespace benchmark_helpers;

// =============================================================================
// Category A: Workflow Happy Path - Success Cases
// Grouped: Coawait -> Throw -> IfElse for same configuration
// =============================================================================

static void BM_ComputeRatio_Coawait_Success(benchmark::State &state)
{
  const auto &[a, b] = kValidInputs[0];
  for (auto _ : state)
  {
    auto result = computeRatioCoawait(a, b);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_ComputeRatio_Throw_Success(benchmark::State &state)
{
  const auto &[a, b] = kValidInputs[0];
  for (auto _ : state)
  {
    try
    {
      double result = computeRatioThrow(a, b);
      benchmark::DoNotOptimize(result);
    }
    catch (...)
    {
      // Should not happen
    }
  }
}

static void BM_ComputeRatio_IfElse_Success(benchmark::State &state)
{
  const auto &[a, b] = kValidInputs[0];
  for (auto _ : state)
  {
    auto result = computeRatioIfElse(a, b);
    benchmark::DoNotOptimize(result);
  }
}

// Register grouped: Success
BENCHMARK(BM_ComputeRatio_Coawait_Success)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ComputeRatio_Throw_Success)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ComputeRatio_IfElse_Success)->Unit(benchmark::kNanosecond);

// =============================================================================
// Category B: Workflow Error Path - Error on First Parse
// Grouped: Coawait -> Throw -> IfElse for same configuration
// =============================================================================

static void BM_ComputeRatio_Coawait_ErrorFirstParse(benchmark::State &state)
{
  const auto &[a, b] = kInvalidFirst;
  for (auto _ : state)
  {
    auto result = computeRatioCoawait(a, b);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_ComputeRatio_Throw_ErrorFirstParse(benchmark::State &state)
{
  const auto &[a, b] = kInvalidFirst;
  for (auto _ : state)
  {
    try
    {
      double result = computeRatioThrow(a, b);
      benchmark::DoNotOptimize(result);
    }
    catch (const std::runtime_error &)
    {
      // Expected
    }
  }
}

static void BM_ComputeRatio_IfElse_ErrorFirstParse(benchmark::State &state)
{
  const auto &[a, b] = kInvalidFirst;
  for (auto _ : state)
  {
    auto result = computeRatioIfElse(a, b);
    benchmark::DoNotOptimize(result);
  }
}

// Register grouped: ErrorFirstParse
BENCHMARK(BM_ComputeRatio_Coawait_ErrorFirstParse)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ComputeRatio_Throw_ErrorFirstParse)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ComputeRatio_IfElse_ErrorFirstParse)->Unit(benchmark::kNanosecond);

// =============================================================================
// Category B: Workflow Error Path - Error on Second Parse
// Grouped: Coawait -> Throw -> IfElse for same configuration
// =============================================================================

static void BM_ComputeRatio_Coawait_ErrorSecondParse(benchmark::State &state)
{
  const auto &[a, b] = kInvalidSecond;
  for (auto _ : state)
  {
    auto result = computeRatioCoawait(a, b);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_ComputeRatio_Throw_ErrorSecondParse(benchmark::State &state)
{
  const auto &[a, b] = kInvalidSecond;
  for (auto _ : state)
  {
    try
    {
      double result = computeRatioThrow(a, b);
      benchmark::DoNotOptimize(result);
    }
    catch (const std::runtime_error &)
    {
      // Expected
    }
  }
}

static void BM_ComputeRatio_IfElse_ErrorSecondParse(benchmark::State &state)
{
  const auto &[a, b] = kInvalidSecond;
  for (auto _ : state)
  {
    auto result = computeRatioIfElse(a, b);
    benchmark::DoNotOptimize(result);
  }
}

// Register grouped: ErrorSecondParse
BENCHMARK(BM_ComputeRatio_Coawait_ErrorSecondParse)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ComputeRatio_Throw_ErrorSecondParse)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ComputeRatio_IfElse_ErrorSecondParse)->Unit(benchmark::kNanosecond);

// =============================================================================
// Category B: Workflow Error Path - Error on Division
// Grouped: Coawait -> Throw -> IfElse for same configuration
// =============================================================================

static void BM_ComputeRatio_Coawait_ErrorDivision(benchmark::State &state)
{
  const auto &[a, b] = kDivByZero;
  for (auto _ : state)
  {
    auto result = computeRatioCoawait(a, b);
    benchmark::DoNotOptimize(result);
  }
}

static void BM_ComputeRatio_Throw_ErrorDivision(benchmark::State &state)
{
  const auto &[a, b] = kDivByZero;
  for (auto _ : state)
  {
    try
    {
      double result = computeRatioThrow(a, b);
      benchmark::DoNotOptimize(result);
    }
    catch (const std::runtime_error &)
    {
      // Expected
    }
  }
}

static void BM_ComputeRatio_IfElse_ErrorDivision(benchmark::State &state)
{
  const auto &[a, b] = kDivByZero;
  for (auto _ : state)
  {
    auto result = computeRatioIfElse(a, b);
    benchmark::DoNotOptimize(result);
  }
}

// Register grouped: ErrorDivision
BENCHMARK(BM_ComputeRatio_Coawait_ErrorDivision)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ComputeRatio_Throw_ErrorDivision)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ComputeRatio_IfElse_ErrorDivision)->Unit(benchmark::kNanosecond);
