// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

// =============================================================================
// Category G: Repeated Recursive Benchmarks
// Calls recursive functions N times with fixed depth=200.
// Compares: ropic::Either (co_await) vs try-catch (Throw) vs std::pair (IfElse)
// =============================================================================

#include <benchmark/benchmark.h>
#include "ropic.hpp"
#include <string>
#include <utility>

using namespace ropic;

// =============================================================================
// Recursive Function Implementations (same as RecursiveBenchmark.cpp)
// =============================================================================

static auto recursiveCoawaitR(int depth, int errorAt) noexcept
    -> Either<int, std::string>
{
  if (errorAt == 0)
  {
    co_return "Error at depth " + std::to_string(depth);
  }
  if (depth == 0)
  {
    co_return depth;
  }
  int result = co_await recursiveCoawaitR(depth - 1, errorAt - 1);
  co_return result;
}

#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
static auto recursiveThrowR(int depth, int errorAt) -> int
{
  if (errorAt == 0)
  {
    throw "Error at depth " + std::to_string(depth);
  }
  if (depth == 0)
  {
    return depth;
  }
  return recursiveThrowR(depth - 1, errorAt - 1);
}

#if defined(_MSC_VER)
__declspec(noinline)
#else
__attribute__((noinline))
#endif
static auto recursiveIfElseR(int depth, int errorAt) noexcept
    -> std::pair<int, std::string>
{
  if (errorAt == 0)
  {
    return {-1, "Error at depth " + std::to_string(depth)};
  }
  if (depth == 0)
  {
    return {depth, ""};
  }
  auto result = recursiveIfElseR(depth - 1, errorAt - 1);
  if (result.first < 0)
  {
    return {-1, result.second};
  }
  return result;
}

// =============================================================================
// Repeated Wrappers: call recursive function N times
// =============================================================================

static void repeatedCoawait(int n, int depth, int errorAt)
{
  for (int i = 0; i < n; ++i)
  {
    auto result = recursiveCoawaitR(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
}

static void repeatedThrow(int n, int depth, int errorAt)
{
  for (int i = 0; i < n; ++i)
  {
    try
    {
      int result = recursiveThrowR(depth, errorAt);
      benchmark::DoNotOptimize(result);
    }
    catch (const std::string&)
    {
    }
  }
}

static void repeatedIfElse(int n, int depth, int errorAt)
{
  for (int i = 0; i < n; ++i)
  {
    auto result = recursiveIfElseR(depth, errorAt);
    benchmark::DoNotOptimize(result);
  }
}

// =============================================================================
// Depth = 100, errorAt computed per error scenario
// =============================================================================

static constexpr int kDepth = 200;

// --- Success (no error) ------------------------------------------------------

static void BM_Repeated_Coawait_Success(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth + 100; // Never triggers error

  for (auto _ : state)
  {
    repeatedCoawait(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_Throw_Success(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth + 100; // Never triggers error

  for (auto _ : state)
  {
    repeatedThrow(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_IfElse_Success(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth + 100; // Never triggers error

  for (auto _ : state)
  {
    repeatedIfElse(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

// --- Early Error (10% depth) -------------------------------------------------

static void BM_Repeated_Coawait_EarlyError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth / 10;

  for (auto _ : state)
  {
    repeatedCoawait(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_Throw_EarlyError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth / 10;

  for (auto _ : state)
  {
    repeatedThrow(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_IfElse_EarlyError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth / 10;

  for (auto _ : state)
  {
    repeatedIfElse(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

// --- Early-Mid Error (30% depth) ---------------------------------------------

static void BM_Repeated_Coawait_EarlyMidError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = (kDepth * 3) / 10;

  for (auto _ : state)
  {
    repeatedCoawait(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_Throw_EarlyMidError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = (kDepth * 3) / 10;

  for (auto _ : state)
  {
    repeatedThrow(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_IfElse_EarlyMidError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = (kDepth * 3) / 10;

  for (auto _ : state)
  {
    repeatedIfElse(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

// --- Mid Error (50% depth) ---------------------------------------------------

static void BM_Repeated_Coawait_MidError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth / 2;

  for (auto _ : state)
  {
    repeatedCoawait(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_Throw_MidError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth / 2;

  for (auto _ : state)
  {
    repeatedThrow(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_IfElse_MidError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = kDepth / 2;

  for (auto _ : state)
  {
    repeatedIfElse(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

// --- Late Error (90% depth) --------------------------------------------------

static void BM_Repeated_Coawait_LateError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = (kDepth * 9) / 10;

  for (auto _ : state)
  {
    repeatedCoawait(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_Throw_LateError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = (kDepth * 9) / 10;

  for (auto _ : state)
  {
    repeatedThrow(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

static void BM_Repeated_IfElse_LateError(benchmark::State& state)
{
  const int n = static_cast<int>(state.range(0));
  const int errorAt = (kDepth * 9) / 10;

  for (auto _ : state)
  {
    repeatedIfElse(n, kDepth, errorAt);
  }
  state.SetItemsProcessed(state.iterations() * n);
}

// =============================================================================
// Registration: Success x {10000, 20000, 40000}
// =============================================================================

BENCHMARK(BM_Repeated_Coawait_Success)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_Success)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_Success)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_Success)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_Success)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_Success)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_Success)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_Success)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_Success)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Registration: Early Error x {10000, 20000, 40000}
// =============================================================================

BENCHMARK(BM_Repeated_Coawait_EarlyError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_EarlyError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_EarlyError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_EarlyError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_EarlyError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_EarlyError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_EarlyError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_EarlyError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_EarlyError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Registration: Early-Mid Error x {10000, 20000, 40000}
// =============================================================================

BENCHMARK(BM_Repeated_Coawait_EarlyMidError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_EarlyMidError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_EarlyMidError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_EarlyMidError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_EarlyMidError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_EarlyMidError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_EarlyMidError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_EarlyMidError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_EarlyMidError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Registration: Mid Error x {10000, 20000, 40000}
// =============================================================================

BENCHMARK(BM_Repeated_Coawait_MidError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_MidError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_MidError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_MidError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_MidError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_MidError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_MidError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_MidError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_MidError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);

// =============================================================================
// Registration: Late Error x {10000, 20000, 40000}
// =============================================================================

BENCHMARK(BM_Repeated_Coawait_LateError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_LateError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_LateError)
    ->Arg(10000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_LateError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_LateError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_LateError)
    ->Arg(20000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Repeated_Coawait_LateError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_Throw_LateError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
BENCHMARK(BM_Repeated_IfElse_LateError)
    ->Arg(40000)
    ->Unit(benchmark::kMillisecond);
