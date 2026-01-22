// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

// =============================================================================
// Category E: Data Payload Size
// Compares: ropic::Either (co_await) vs try-catch (Throw) vs std::optional (IfElse)
// =============================================================================

#include <benchmark/benchmark.h>

#include <array>
#include <optional>
#include <stdexcept>
#include <string>

#include "ropic.hpp"

using namespace ropic;

// =============================================================================
// Payload Types
// =============================================================================
// NOLINTBEGIN(readability-magic-numbers)
struct SmallPayload
{
  double value;
};

struct MediumPayload
{
  std::array<double, 8> values;
};

struct LargePayload
{
  std::array<double, 64> values;
};

// =============================================================================
// Small Payload (8 bytes) - Coawait
// =============================================================================

auto computeSmallPayloadCoawait(double x) noexcept
    -> Either<SmallPayload, std::string>
{
  if (x < 0)
  {
    co_return std::string("Negative value");
  }
  co_return SmallPayload{x * 2.0};
}

// =============================================================================
// Small Payload (8 bytes) - Throw
// =============================================================================

auto computeSmallPayloadThrow(double x) -> SmallPayload
{
  if (x < 0)
  {
    throw std::runtime_error("Negative value");
  }
  return SmallPayload{x * 2.0};
}

// =============================================================================
// Small Payload (8 bytes) - IfElse
// =============================================================================

auto computeSmallPayloadIfElse(double x) noexcept -> std::optional<SmallPayload>
{
  if (x < 0)
  {
    return std::nullopt;
  }
  return SmallPayload{x * 2.0};
}

// =============================================================================
// Medium Payload (64 bytes) - Coawait
// =============================================================================

auto computeMediumPayloadCoawait(double x) noexcept
    -> Either<MediumPayload, std::string>
{
  if (x < 0)
  {
    co_return std::string("Negative value");
  }
  MediumPayload result;
  for (size_t i = 0; i < result.values.size(); ++i)
  {
    result.values[i] = x + static_cast<double>(i);
  }
  co_return result;
}

// =============================================================================
// Medium Payload (64 bytes) - Throw
// =============================================================================

auto computeMediumPayloadThrow(double x) -> MediumPayload
{
  if (x < 0)
  {
    throw std::runtime_error("Negative value");
  }
  MediumPayload result;
  for (size_t i = 0; i < result.values.size(); ++i)
  {
    result.values[i] = x + static_cast<double>(i);
  }
  return result;
}

// =============================================================================
// Medium Payload (64 bytes) - IfElse
// =============================================================================

auto computeMediumPayloadIfElse(double x) noexcept
    -> std::optional<MediumPayload>
{
  if (x < 0)
  {
    return std::nullopt;
  }
  MediumPayload result;
  for (size_t i = 0; i < result.values.size(); ++i)
  {
    result.values[i] = x + static_cast<double>(i);
  }
  return result;
}

// =============================================================================
// Large Payload (512 bytes) - Coawait
// =============================================================================

auto computeLargePayloadCoawait(double x) noexcept
    -> Either<LargePayload, std::string>
{
  if (x < 0)
  {
    co_return std::string("Negative value");
  }
  LargePayload result;
  for (size_t i = 0; i < result.values.size(); ++i)
  {
    result.values[i] = x + static_cast<double>(i);
  }
  co_return result;
}

// =============================================================================
// Large Payload (512 bytes) - Throw
// =============================================================================

auto computeLargePayloadThrow(double x) -> LargePayload
{
  if (x < 0)
  {
    throw std::runtime_error("Negative value");
  }
  LargePayload result;
  for (size_t i = 0; i < result.values.size(); ++i)
  {
    result.values[i] = x + static_cast<double>(i);
  }
  return result;
}

// =============================================================================
// Large Payload (512 bytes) - IfElse
// =============================================================================

auto computeLargePayloadIfElse(double x) noexcept -> std::optional<LargePayload>
{
  if (x < 0)
  {
    return std::nullopt;
  }
  LargePayload result;
  for (size_t i = 0; i < result.values.size(); ++i)
  {
    result.values[i] = x + static_cast<double>(i);
  }
  return result;
}

// =============================================================================
// Benchmark Functions
// =============================================================================

static void BM_DataPayload_Coawait(benchmark::State &state)
{
  const int size = static_cast<int>(state.range(0));

  for (auto _ : state)
  {
    switch (size)
    {
      case 8:
      {
        auto result = computeSmallPayloadCoawait(1.0);
        benchmark::DoNotOptimize(result);
        break;
      }
      case 64:
      {
        auto result = computeMediumPayloadCoawait(1.0);
        benchmark::DoNotOptimize(result);
        break;
      }
      case 512:
      {
        auto result = computeLargePayloadCoawait(1.0);
        benchmark::DoNotOptimize(result);
        break;
      }
    }
  }
  state.SetBytesProcessed(state.iterations() * size);
}

static void BM_DataPayload_Throw(benchmark::State &state)
{
  const int size = static_cast<int>(state.range(0));

  for (auto _ : state)
  {
    try
    {
      switch (size)
      {
        case 8:
        {
          auto result = computeSmallPayloadThrow(1.0);
          benchmark::DoNotOptimize(result);
          break;
        }
        case 64:
        {
          auto result = computeMediumPayloadThrow(1.0);
          benchmark::DoNotOptimize(result);
          break;
        }
        case 512:
        {
          auto result = computeLargePayloadThrow(1.0);
          benchmark::DoNotOptimize(result);
          break;
        }
      }
    }
    catch (...)
    {
    }
  }
  state.SetBytesProcessed(state.iterations() * size);
}

static void BM_DataPayload_IfElse(benchmark::State &state)
{
  const int size = static_cast<int>(state.range(0));

  for (auto _ : state)
  {
    switch (size)
    {
      case 8:
      {
        auto result = computeSmallPayloadIfElse(1.0);
        benchmark::DoNotOptimize(result);
        break;
      }
      case 64:
      {
        auto result = computeMediumPayloadIfElse(1.0);
        benchmark::DoNotOptimize(result);
        break;
      }
      case 512:
      {
        auto result = computeLargePayloadIfElse(1.0);
        benchmark::DoNotOptimize(result);
        break;
      }
    }
  }
  state.SetBytesProcessed(state.iterations() * size);
}

// Register grouped by payload size: Coawait/8 -> Throw/8 -> IfElse/8 -> Coawait/64 -> ...
BENCHMARK(BM_DataPayload_Coawait)->Arg(8)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_DataPayload_Throw)->Arg(8)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_DataPayload_IfElse)->Arg(8)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_DataPayload_Coawait)->Arg(64)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_DataPayload_Throw)->Arg(64)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_DataPayload_IfElse)->Arg(64)->Unit(benchmark::kNanosecond);

BENCHMARK(BM_DataPayload_Coawait)->Arg(512)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_DataPayload_Throw)->Arg(512)->Unit(benchmark::kNanosecond);
BENCHMARK(BM_DataPayload_IfElse)->Arg(512)->Unit(benchmark::kNanosecond);
// NOLINTEND(readability-magic-numbers)