// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

// =============================================================================
// Category D: Chain Length Scaling
// Compares: ropic::Either (co_await) vs try-catch (Throw) vs std::optional
// (IfElse)
// =============================================================================

#include <benchmark/benchmark.h>
#include <optional>

#include "core/either.hpp"

using namespace ropic;

// =============================================================================
// Chained Operations - Coawait Approach
// =============================================================================

// NOLINTBEGIN(readability-magic-numbers)
template <int DEPTH>
static auto chainedCoawait(double value) noexcept -> Either<double, std::string>
{
  if constexpr (DEPTH == 0)
  {
    co_return value;
  }
  else
  {
    double result = co_await chainedCoawait<DEPTH - 1>(value + 1.0);
    co_return result;
  }
}

// Explicit instantiations
template Either<double, std::string> chainedCoawait<3>(double) noexcept;
template Either<double, std::string> chainedCoawait<5>(double) noexcept;
template Either<double, std::string> chainedCoawait<10>(double) noexcept;
template Either<double, std::string> chainedCoawait<20>(double) noexcept;

// =============================================================================
// Chained Operations - Throw Approach
// =============================================================================

template <int DEPTH>
auto chainedThrow(double value) -> double
{
  if constexpr (DEPTH == 0)
  {
    return value;
  }
  else
  {
    return chainedThrow<DEPTH - 1>(value + 1.0);
  }
}

// =============================================================================
// Chained Operations - IfElse Approach
// =============================================================================

template <int DEPTH>
auto chainedIfElse(double value) noexcept -> std::optional<double>
{
  if constexpr (DEPTH == 0)
  {
    return value;
  }
  else
  {
    auto result = chainedIfElse<DEPTH - 1>(value + 1.0);
    if (!result.has_value())
    {
      return std::nullopt;
    }
    return *result;
  }
}

// =============================================================================
// Benchmark Functions
// =============================================================================

static void BM_ChainedOperations_Coawait(benchmark::State& state)
{
  const int depth = static_cast<int>(state.range(0));

  for (auto _ : state)
  {
    Either<double, std::string> result{0.0};
    switch (depth)
    {
    case 3:
      result = chainedCoawait<3>(1.0);
      break;
    case 5:
      result = chainedCoawait<5>(1.0);
      break;
    case 10:
      result = chainedCoawait<10>(1.0);
      break;
    case 20:
      result = chainedCoawait<20>(1.0);
      break;
    }
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * depth);
}

static void BM_ChainedOperations_Throw(benchmark::State& state)
{
  const int depth = static_cast<int>(state.range(0));

  for (auto _ : state)
  {
    double result = 0.0;
    try
    {
      switch (depth)
      {
      case 3:
        result = chainedThrow<3>(1.0);
        break;
      case 5:
        result = chainedThrow<5>(1.0);
        break;
      case 10:
        result = chainedThrow<10>(1.0);
        break;
      case 20:
        result = chainedThrow<20>(1.0);
        break;
      }
    }
    catch (...)
    {
    }
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * depth);
}

static void BM_ChainedOperations_IfElse(benchmark::State& state)
{
  const int depth = static_cast<int>(state.range(0));

  for (auto _ : state)
  {
    std::optional<double> result;
    switch (depth)
    {
    case 3:
      result = chainedIfElse<3>(1.0);
      break;
    case 5:
      result = chainedIfElse<5>(1.0);
      break;
    case 10:
      result = chainedIfElse<10>(1.0);
      break;
    case 20:
      result = chainedIfElse<20>(1.0);
      break;
    }
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * depth);
}

// Register grouped by chain depth: Coawait/3 -> Throw/3 -> IfElse/3 ->
// Coawait/5 -> ...
BENCHMARK(BM_ChainedOperations_Coawait)->Arg(3)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainedOperations_Throw)->Arg(3)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainedOperations_IfElse)->Arg(3)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ChainedOperations_Coawait)->Arg(5)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainedOperations_Throw)->Arg(5)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainedOperations_IfElse)->Arg(5)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ChainedOperations_Coawait)->Arg(10)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainedOperations_Throw)->Arg(10)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainedOperations_IfElse)->Arg(10)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_ChainedOperations_Coawait)->Arg(20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainedOperations_Throw)->Arg(20)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_ChainedOperations_IfElse)->Arg(20)->Unit(benchmark::kNanosecond);
// NOLINTEND(readability-magic-numbers)