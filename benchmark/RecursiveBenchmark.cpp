// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

// =============================================================================
// Category F: Recursive Depth Benchmarks
// Compares: ropic::Either (co_await) vs try-catch (Throw) vs std::pair (IfElse)
// =============================================================================

#include <benchmark/benchmark.h>
#include "ropic.hpp"
#include <string>
#include <utility>

using namespace ropic;

// =============================================================================
// Recursive Function Implementations
// =============================================================================

/**
 * @brief Recursive function using co_await for error propagation.
 *
 * @param depth Controls the recursion depth (decrements toward 0)
 * @param errorAt Specifies the depth at which an error occurs (decrements toward 0)
 * @return Either<int, std::string> Success with depth value, or error string
 */
Either<int, std::string> recursiveCoawait(int depth, int errorAt) noexcept
{
  if (errorAt == 0)
  {
    co_return "Error at depth " + std::to_string(depth);
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
 */
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
int recursiveThrow(int depth, int errorAt)
{
  if (errorAt == 0)
  {
    throw "Error at depth " + std::to_string(depth);
  }
  if (depth == 0)
  {
    return depth;
  }
  return recursiveThrow(depth - 1, errorAt - 1);
}

/**
 * @brief Recursive function using if-else checks for error propagation.
 *
 * @param depth Controls the recursion depth (decrements toward 0)
 * @param errorAt Specifies the depth at which an error occurs (decrements toward 0)
 * @return std::pair<int, std::string> Success: {value, ""}, Error: {-1, errorMessage}
 */
#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
std::pair<int, std::string> recursiveIfElse(int depth, int errorAt) noexcept
{
  if (errorAt == 0)
  {
    return {-1, "Error at depth " + std::to_string(depth)};
  }
  if (depth == 0)
  {
    return {depth, ""};
  }
  auto result = recursiveIfElse(depth - 1, errorAt - 1);
  if (result.first < 0)
  {
    return {-1, result.second};
  }
  return result;
}

// =============================================================================
// Benchmark: Success Path (no errors)
// Grouped by depth: Coawait/N -> Throw/N -> IfElse/N
// =============================================================================

static void BM_Recursive_Coawait_Success(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth + 100; // Never triggers error

  for (auto _ : state)
  {
    auto result = recursiveCoawait(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * depth);
}

static void BM_Recursive_Throw_Success(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth + 100; // Never triggers error

  for (auto _ : state)
  {
    try
    {
      int result = recursiveThrow(depth, errorAt);
      benchmark::DoNotOptimize(result);
      benchmark::ClobberMemory();
    }
    catch (...)
    {
      // Should not happen in success path
    }
  }
  state.SetItemsProcessed(state.iterations() * depth);
}

static void BM_Recursive_IfElse_Success(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth + 100; // Never triggers error

  for (auto _ : state)
  {
    auto result = recursiveIfElse(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * depth);
}

// Register grouped by depth: Coawait/10 -> Throw/10 -> IfElse/10 -> Coawait/50 -> ...
BENCHMARK(BM_Recursive_Coawait_Success)->Arg(10)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_Success)->Arg(10)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_Success)->Arg(10)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_Success)->Arg(50)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_Success)->Arg(50)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_Success)->Arg(50)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_Success)->Arg(100)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_Success)->Arg(100)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_Success)->Arg(100)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_Success)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_Success)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_Success)->Arg(200)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_Success)->Arg(300)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_Success)->Arg(300)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_Success)->Arg(300)->Unit(benchmark::kMicrosecond);

// =============================================================================
// Benchmark: Early Error (error at 10% depth)
// Grouped by depth: Coawait/N -> Throw/N -> IfElse/N
// =============================================================================

static void BM_Recursive_Coawait_EarlyError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth / 10; // Error at 10% depth

  for (auto _ : state)
  {
    auto result = recursiveCoawait(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

static void BM_Recursive_Throw_EarlyError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth / 10; // Error at 10% depth

  for (auto _ : state)
  {
    try
    {
      int result = recursiveThrow(depth, errorAt);
      benchmark::DoNotOptimize(result);
    }
    catch (const std::string &)
    {
      // Expected error path
    }
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

static void BM_Recursive_IfElse_EarlyError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth / 10; // Error at 10% depth

  for (auto _ : state)
  {
    auto result = recursiveIfElse(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

// Register grouped by depth
BENCHMARK(BM_Recursive_Coawait_EarlyError)->Arg(100)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_EarlyError)->Arg(100)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_EarlyError)->Arg(100)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_EarlyError)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_EarlyError)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_EarlyError)->Arg(200)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_EarlyError)->Arg(300)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_EarlyError)->Arg(300)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_EarlyError)->Arg(300)->Unit(benchmark::kMicrosecond);

// =============================================================================
// Benchmark: Mid Error (error at 50% depth)
// Grouped by depth: Coawait/N -> Throw/N -> IfElse/N
// =============================================================================

static void BM_Recursive_Coawait_MidError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth / 2; // Error at 50% depth

  for (auto _ : state)
  {
    auto result = recursiveCoawait(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

static void BM_Recursive_Throw_MidError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth / 2; // Error at 50% depth

  for (auto _ : state)
  {
    try
    {
      int result = recursiveThrow(depth, errorAt);
      benchmark::DoNotOptimize(result);
    }
    catch (const std::string &)
    {
      // Expected error path
    }
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

static void BM_Recursive_IfElse_MidError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = depth / 2; // Error at 50% depth

  for (auto _ : state)
  {
    auto result = recursiveIfElse(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

// Register grouped by depth
BENCHMARK(BM_Recursive_Coawait_MidError)->Arg(100)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_MidError)->Arg(100)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_MidError)->Arg(100)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_MidError)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_MidError)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_MidError)->Arg(200)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_MidError)->Arg(300)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_MidError)->Arg(300)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_MidError)->Arg(300)->Unit(benchmark::kMicrosecond);

// =============================================================================
// Benchmark: Late Error (error at 90% depth)
// Grouped by depth: Coawait/N -> Throw/N -> IfElse/N
// =============================================================================

static void BM_Recursive_Coawait_LateError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = (depth * 9) / 10; // Error at 90% depth

  for (auto _ : state)
  {
    auto result = recursiveCoawait(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

static void BM_Recursive_Throw_LateError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = (depth * 9) / 10; // Error at 90% depth

  for (auto _ : state)
  {
    try
    {
      int result = recursiveThrow(depth, errorAt);
      benchmark::DoNotOptimize(result);
    }
    catch (const std::string &)
    {
      // Expected error path
    }
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

static void BM_Recursive_IfElse_LateError(benchmark::State &state)
{
  const int depth = static_cast<int>(state.range(0));
  const int errorAt = (depth * 9) / 10; // Error at 90% depth

  for (auto _ : state)
  {
    auto result = recursiveIfElse(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
  state.SetItemsProcessed(state.iterations() * errorAt);
}

// Register grouped by depth
BENCHMARK(BM_Recursive_Coawait_LateError)->Arg(100)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_LateError)->Arg(100)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_LateError)->Arg(100)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_LateError)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_LateError)->Arg(200)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_LateError)->Arg(200)->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Recursive_Coawait_LateError)->Arg(300)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_Throw_LateError)->Arg(300)->Unit(benchmark::kMicrosecond);
BENCHMARK(BM_Recursive_IfElse_LateError)->Arg(300)->Unit(benchmark::kMicrosecond);
