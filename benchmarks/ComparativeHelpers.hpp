// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <charconv>
#include <optional>
#include <stdexcept>
#include <string>

#include "ropic.hpp"

namespace benchmark_helpers
{

using namespace ropic;

// =============================================================================
// Helper Functions - ropic::Either Approach (Coawait)
// =============================================================================

inline auto parseDoubleCoawait(const std::string& s) noexcept
    -> Either<double, std::string>
{
  if (s.empty())
  {
    co_return std::string("Empty string");
  }

  double value = 0.0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

  if (ec != std::errc{} || ptr != s.data() + s.size())
  {
    co_return "Invalid number: " + s;
  }
  co_return value;
}

inline auto divideCoawait(double a, double b) noexcept
    -> Either<double, std::string>
{
  if (b == 0.0)
  {
    co_return std::string("Division by zero");
  }
  co_return a / b;
}

inline auto
computeRatioCoawait(const std::string& a, const std::string& b) noexcept
    -> Either<double, std::string>
{
  double x = co_await parseDoubleCoawait(a);
  double y = co_await parseDoubleCoawait(b);
  double z = co_await divideCoawait(x, y);
  co_return z;
}

// =============================================================================
// Helper Functions - Exception Approach (Throw)
// =============================================================================

inline auto parseDoubleThrow(const std::string& s) -> double
{
  if (s.empty())
  {
    throw std::runtime_error("Empty string");
  }

  double value = 0.0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

  if (ec != std::errc{} || ptr != s.data() + s.size())
  {
    throw std::runtime_error("Invalid number: " + s);
  }
  return value;
}

inline auto divideThrow(double a, double b) -> double
{
  if (b == 0.0)
  {
    throw std::runtime_error("Division by zero");
  }
  return a / b;
}

inline auto computeRatioThrow(const std::string& a, const std::string& b)
    -> double
{
  double x = parseDoubleThrow(a);
  double y = parseDoubleThrow(b);
  return divideThrow(x, y);
}

// =============================================================================
// Helper Functions - If-Else Approach (Optional)
// =============================================================================

inline auto parseDoubleIfElse(const std::string& s) noexcept
    -> std::optional<double>
{
  if (s.empty())
  {
    return std::nullopt;
  }

  double value = 0.0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);

  if (ec != std::errc{} || ptr != s.data() + s.size())
  {
    return std::nullopt;
  }
  return value;
}

inline auto divideIfElse(double a, double b) noexcept -> std::optional<double>
{
  if (b == 0.0)
  {
    return std::nullopt;
  }
  return a / b;
}

inline auto
computeRatioIfElse(const std::string& a, const std::string& b) noexcept
    -> std::optional<double>
{
  auto x = parseDoubleIfElse(a);
  if (!x.has_value())
  {
    return std::nullopt;
  }

  auto y = parseDoubleIfElse(b);
  if (!y.has_value())
  {
    return std::nullopt;
  }

  return divideIfElse(*x, *y);
}

// =============================================================================
// Test Data
// =============================================================================

// Valid inputs (happy path)
constexpr std::array<std::pair<const char*, const char*>, 3> kValidInputs = {{
    {"3.14159", "2.0"},
    {"100.0", "4.0"},
    {"1.0", "3.0"},
}};

// Invalid inputs (error path)
constexpr std::pair<const char*, const char*> kInvalidFirst = {"abc", "2.0"};
constexpr std::pair<const char*, const char*> kInvalidSecond = {"3.14", "xyz"};
constexpr std::pair<const char*, const char*> kDivByZero = {"3.14", "0.0"};

} // namespace benchmark_helpers
