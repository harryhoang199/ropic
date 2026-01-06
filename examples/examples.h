#pragma once

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "Result.hpp"

// ==========================================
// USAGE EXAMPLES (COROUTINE FUNCTIONS)
// ==========================================
using namespace ropic;

/**
 * @brief Trims leading and trailing whitespace from a string.
 * @param str The string to trim.
 * @return Either containing the trimmed string or an error if empty.
 */
inline auto trim(const std::string& str) -> Result<std::string>
{
  const auto start = str.find_first_not_of(" \t\n\r\f\v");
  if (start == std::string::npos)
    co_return {ErrorTag::VALIDATION, "String is empty after trimming"};

  const auto end = str.find_last_not_of(" \t\n\r\f\v");
  co_return str.substr(start, end - start + 1);
}

/**
 * @brief Parses a string to a double.
 * @param str The string to parse.
 * @return Either containing the parsed double or an error.
 */
inline auto parseDouble(std::string str) -> Result<double>
{
  try
  {
    auto trimEither = trim(str);
    std::string& trimmed1 = co_await trimEither;

    co_return std::stod(trimmed1);
  }
  catch (...)
  {
    Error e{ErrorTag::VALIDATION, "Cannot parse '" + str + "' to double"};
    co_return e;
  }
}

/**
 * @brief Divides two numbers.
 * @param numerator The numerator.
 * @param denominator The denominator.
 * @return Either containing the result or an error.
 */
inline auto divide(double numerator, double denominator) -> Result<double>
{
  if (denominator == 0.0)
  {
    Error e{ErrorTag::VALIDATION, "Cannot divide by 0"};
    co_return e;
  }

  co_return (numerator / denominator);
}

/**
 * @brief Divides two numbers provided as strings.
 * @param numeratorStr String representation of numerator.
 * @param denominatorStr String representation of denominator.
 * @return Either containing the result or an error.
 */
inline auto divideStr(
    const std::string& numeratorStr, // NOLINT
    const std::string& denominatorStr) -> Result<double>
{
  std::cout << "x = " << numeratorStr << ", y = " << denominatorStr << "\n";

  auto xEither = parseDouble(numeratorStr);
  double x = co_await xEither;

  auto yEither = parseDouble(denominatorStr);
  if (auto err = yEither.error())
    co_return *err;
  double y = *(yEither.data());

  double result = co_await divide(x, y);

  co_return result;
}

// ==========================================
// Result<Void> EXAMPLES - Operations that succeed or fail without returning
// data
// ==========================================

/**
 * @brief Validates that a number is positive.
 * @param value The value to validate.
 * @return Result<Void> - OK on success, error if validation fails.
 */
inline auto validatePositive(double value) -> Result<Void>
{
  if (value <= 0)
    co_return {
        ErrorTag::VALIDATION,
        "Value must be positive, got: " + std::to_string(value)};
  co_return OK;
}

/**
 * @brief Validates that a string is not empty.
 * @param str The string to validate.
 * @return Result<Void> - OK on success, error if empty.
 */
inline auto validateNotEmpty(const std::string& str) -> Result<Void>
{
  if (str.empty())
    co_return {ErrorTag::VALIDATION, "String cannot be empty"};
  co_return OK;
}

/**
 * @brief Simulates saving data to storage.
 * @param filename The target filename.
 * @param data The data to save.
 * @return Result<Void> - OK on success, error on failure.
 */
inline auto saveToStorage(const std::string& filename, double data)
    -> Result<Void>
{
  // Validate inputs first (Result<Void> in Result<Void>)
  co_await validateNotEmpty(filename);

  // Simulate file operation that could fail
  if (filename.find("..") != std::string::npos)
    co_return {
        ErrorTag::VALIDATION, "Invalid filename: path traversal detected"};

  std::cout << "Saved " << data << " to " << filename << "\n";
  co_return OK;
}

// ==========================================
// Result<OtherType> using Result<Void> - Validate before computing
// ==========================================

/**
 * @brief Computes the square root with validation.
 * @param value The value to compute square root of.
 * @return Result<double> - The square root or an error.
 *
 * Demonstrates: Using Result<Void> validation within Result<double>.
 */
inline auto safeSqrt(double value) -> Result<double>
{
  // Use Result<Void> for validation before computing
  co_await validatePositive(value);

  co_return std::sqrt(value);
}

/**
 * @brief Computes the natural logarithm with validation.
 * @param value The value to compute log of.
 * @return Result<double> - The natural log or an error.
 */
inline auto safeLog(double value) -> Result<double>
{
  co_await validatePositive(value);
  co_return std::log(value);
}

/**
 * @brief Parses and validates a positive number from string.
 * @param str The string to parse.
 * @return Result<double> - The parsed positive number or an error.
 *
 * Demonstrates: Chaining Result<double> then Result<Void> validation.
 */
inline auto parsePositiveDouble(const std::string& str) -> Result<double>
{
  // First parse (Result<double>)
  double value = co_await parseDouble(str);

  // Then validate (Result<Void>)
  co_await validatePositive(value);

  co_return value;
}

// ==========================================
// Result<Void> using Result<OtherType> - Process data, report success/failure
// ==========================================

/**
 * @brief Processes two numbers and saves the result.
 * @param numeratorStr Numerator as string.
 * @param denominatorStr Denominator as string.
 * @param filename Output filename.
 * @return Result<Void> - OK on success, error on failure.
 *
 * Demonstrates: Using Result<double> operations within Result<Void>.
 */
inline auto processAndSave(
    const std::string& numeratorStr,
    const std::string& denominatorStr, // NOLINT
    const std::string& filename) -> Result<Void>
{
  // Perform computation (Result<double> in Result<Void>)
  double result = co_await divideStr(numeratorStr, denominatorStr);

  // Save the result (Result<Void> in Result<Void>)
  co_await saveToStorage(filename, result);

  co_return OK;
}

/**
 * @brief Validates a mathematical expression can be computed.
 * @param base The base value.
 * @param exponent The exponent.
 * @return Result<Void> - OK if computable, error otherwise.
 *
 * Demonstrates: Using multiple Result<OtherType> to validate without keeping
 * results.
 */
inline auto validateComputable(double base, double exponent) -> Result<Void>
{
  // Check sqrt is valid (uses the computed value but discards it)
  (void)co_await safeSqrt(base);

  // Check log is valid
  (void)co_await safeLog(exponent);

  // Check division is valid
  (void)co_await divide(base, exponent);

  co_return OK;
}

// ==========================================
// COMPLEX COMPOSITION EXAMPLE
// ==========================================

/**
 * @brief Computes a weighted average with full validation.
 * @param values String representations of values.
 * @param weights The weights for averaging.
 * @return Result<double> - The weighted average or an error.
 *
 * Demonstrates: Complex composition of Result<Void> and Result<double>.
 */
inline auto computeWeightedAverage(
    const std::vector<std::string>& values, const std::vector<double>& weights)
    -> Result<double>
{
  if (values.size() != weights.size())
    co_return {ErrorTag::VALIDATION, "Values and weights must have same size"};

  if (values.empty())
    co_return {ErrorTag::VALIDATION, "Cannot compute average of empty list"};

  double sum = 0.0;
  double weightSum = 0.0;

  for (size_t i = 0; i < values.size(); ++i)
  {
    // Validate weight is positive (Result<Void> in Result<double>)
    co_await validatePositive(weights[i]);

    // Parse value (Result<double>)
    double val = co_await parseDouble(values[i]);

    sum += val * weights[i];
    weightSum += weights[i];
  }

  // Compute final average (Result<double>)
  double average = co_await divide(sum, weightSum);

  co_return average;
}

/**
 * @brief Batch processes multiple calculations and reports overall success.
 * @param inputs Pairs of numerator/denominator strings.
 * @return Result<Void> - OK if all succeed, first error otherwise.
 *
 * Demonstrates: Aggregating multiple Result<double> into Result<Void>.
 */
inline auto
batchProcess(const std::vector<std::pair<std::string, std::string>>& inputs)
    -> Result<Void>
{
  for (const auto& [num, den] : inputs)
  {
    // Each division returns Result<double>, but we only care about success
    (void)co_await divideStr(num, den);
  }

  std::cout << "Successfully processed " << inputs.size() << " calculations\n";
  co_return OK;
}

// ==========================================
// ASYNC INTEGRATION EXAMPLES
// ==========================================
// Demonstrates co_await on non-Either awaitables (e.g., AsyncFetch)
// within Either-returning coroutines.

#include "tasks.hpp"

/**
 * @brief Simulates async fetch of both operands, then divides.
 * @param numeratorStr The numerator value to "fetch" asynchronously.
 * @param denominatorStr The denominator value to "fetch" asynchronously.
 * @return Result<double> - The division result or an error.
 *
 * Demonstrates: Multiple co_await on non-Either awaitables (AsyncFetch)
 * within an Either coroutine. The Either promise's pass-through
 * await_transform handles foreign awaitables seamlessly.
 *
 * Each AsyncFetch simulates an async operation (e.g., network fetch, file I/O)
 * with random latency. Both operands are fetched sequentially, then parsed
 * and divided using standard Either operations with automatic error
 * propagation.
 */
inline auto asyncDivideStr(
    std::string numeratorStr,
    std::string denominatorStr) -> Result<double>
{
  // co_await non-Either awaitables - works because EitherPromise
  // has a pass-through await_transform for non-Either types.
  // Simulate fetching both operands from async sources.
  std::string fetchedNumerator =
      co_await examples::AsyncFetch{std::move(numeratorStr)};

  std::string fetchedDenominator =
      co_await examples::AsyncFetch{std::move(denominatorStr)};

  // Now use standard Either operations - errors propagate automatically
  double result = co_await divideStr(fetchedNumerator, fetchedDenominator);

  co_return result;
}