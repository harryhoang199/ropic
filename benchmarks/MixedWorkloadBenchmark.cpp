// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

// =============================================================================
// Category C: Mixed Error Rates
// Compares: ropic::Either (co_await) vs try-catch (Throw) vs std::optional (IfElse)
// =============================================================================

#include <benchmark/benchmark.h>
#include "ComparativeHelpers.hpp"
#include <random>

using namespace benchmark_helpers;

// =============================================================================
// Mixed Workload Benchmarks - Grouped by Error Rate
// Each error rate shows: Coawait -> Throw -> IfElse
// =============================================================================

static void BM_MixedWorkload_Coawait(benchmark::State &state)
{
  const int errorRate = static_cast<int>(state.range(0)); // 1, 10, 25, 50
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 99);

  for (auto _ : state)
  {
    bool shouldError = dist(rng) < errorRate;
    if (shouldError)
    {
      auto result = computeRatioCoawait("abc", "2.0");
      benchmark::DoNotOptimize(result);
    }
    else
    {
      auto result = computeRatioCoawait("3.14159", "2.0");
      benchmark::DoNotOptimize(result);
    }
  }
}

static void BM_MixedWorkload_Throw(benchmark::State &state)
{
  const int errorRate = static_cast<int>(state.range(0));
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 99);

  for (auto _ : state)
  {
    bool shouldError = dist(rng) < errorRate;
    try
    {
      if (shouldError)
      {
        double result = computeRatioThrow("abc", "2.0");
        benchmark::DoNotOptimize(result);
      }
      else
      {
        double result = computeRatioThrow("3.14159", "2.0");
        benchmark::DoNotOptimize(result);
      }
    }
    catch (const std::runtime_error &)
    {
      // Expected for error cases
    }
  }
}

static void BM_MixedWorkload_IfElse(benchmark::State &state)
{
  const int errorRate = static_cast<int>(state.range(0));
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 99);

  for (auto _ : state)
  {
    bool shouldError = dist(rng) < errorRate;
    if (shouldError)
    {
      auto result = computeRatioIfElse("abc", "2.0");
      benchmark::DoNotOptimize(result);
    }
    else
    {
      auto result = computeRatioIfElse("3.14159", "2.0");
      benchmark::DoNotOptimize(result);
    }
  }
}

// Register grouped by error rate: Coawait/1 -> Throw/1 -> IfElse/1 -> Coawait/10 -> ...
// Note: Google Benchmark sorts by registration order, so we register in groups
BENCHMARK(BM_MixedWorkload_Coawait)->Arg(1)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MixedWorkload_Throw)->Arg(1)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MixedWorkload_IfElse)->Arg(1)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_MixedWorkload_Coawait)->Arg(10)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MixedWorkload_Throw)->Arg(10)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MixedWorkload_IfElse)->Arg(10)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_MixedWorkload_Coawait)->Arg(25)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MixedWorkload_Throw)->Arg(25)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MixedWorkload_IfElse)->Arg(25)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_MixedWorkload_Coawait)->Arg(50)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MixedWorkload_Throw)->Arg(50)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_MixedWorkload_IfElse)->Arg(50)->Unit(benchmark::kNanosecond);
