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
 * @brief Parses a string to a double.
 * @param str The string to parse.
 * @return Either containing the parsed double or an error.
 */
inline Result<double> parseDouble(std::string str)
{
  try
  {
    co_return std::stod(str);
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
inline Result<double> divide(double numerator, double denominator)
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
inline Result<double> divideStr(
    const std::string& numeratorStr, // NOLINT
    const std::string& denominatorStr)
{
  // co_await will automatically extract the value or return the error via
  // await_transform
  auto xEither = parseDouble(numeratorStr);
  double x = co_await xEither;

  auto yEither = parseDouble(denominatorStr);
  if (auto err = yEither.error())
    co_return *err;
  double y = *(yEither.data());

  double result = co_await divide(x, y);

  std::cout << "x = " << x << ", y = " << y << ", result = " << result << "\n";

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
inline Result<Void> validatePositive(double value)
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
inline Result<Void> validateNotEmpty(const std::string& str)
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
inline Result<Void> saveToStorage(const std::string& filename, double data)
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
inline Result<double> safeSqrt(double value)
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
inline Result<double> safeLog(double value)
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
inline Result<double> parsePositiveDouble(const std::string& str)
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
inline Result<Void> processAndSave(
    const std::string& numeratorStr,
    const std::string& denominatorStr, // NOLINT
    const std::string& filename)
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
inline Result<Void> validateComputable(double base, double exponent)
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
inline Result<double> computeWeightedAverage(
    const std::vector<std::string>& values, const std::vector<double>& weights)
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
inline Result<Void>
batchProcess(const std::vector<std::pair<std::string, std::string>>& inputs)
{
  for (const auto& [num, den] : inputs)
  {
    // Each division returns Result<double>, but we only care about success
    (void)co_await divideStr(num, den);
  }

  std::cout << "Successfully processed " << inputs.size() << " calculations\n";
  co_return OK;
}