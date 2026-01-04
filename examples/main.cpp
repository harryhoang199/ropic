// Railway Oriented Programming with C++20 Coroutines
//
// File organization:
//   - src/error.h     : Error types (ErrorTag, Error) and utilities
//   - src/either.h    : Either<ERROR, DATA> coroutine template for ROP
//   - src/examples.h  : Example coroutine functions demonstrating usage
//   - main.cpp        : Test cases and main function (this file)

#include <iostream>

#include "examples.h"

namespace
{
void printSuccess(const std::string& msg)
{
  std::cout << "[OK] " << msg << "\n";
}

void printError(const Error& err)
{
  std::cout << "[FAIL] " << err.message() << " (tag: " << toString(err.tag())
            << ")\n";
}
} // namespace
// NOLINTBEGIN(readability-magic-numbers)
int main()
{
  std::cout << "=== Testing Railway Oriented Programming ===\n\n";

  // ==========================================
  // Basic Result<double> tests
  // ==========================================
  std::cout << "--- Basic Result<double> ---\n";

  std::cout << "Test 1: divideStr(\"10.2\", \"5\") - success case\n";
  auto task1 = divideStr("10.2", "5");
  if (auto err = task1.error())
    printError(*err);
  else
    printSuccess("Result: " + std::to_string(*task1.data()));
  std::cout << "\n";

  std::cout << "Test 2: divideStr(\".2\", \"0\") - division by zero\n";
  auto task2 = divideStr(".2", "0");
  if (auto err = task2.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 3: divideStr(\"abc\", \"5\") - parse error\n";
  auto task3 = divideStr("abc", "5");
  if (auto err = task3.error())
    printError(*err);
  std::cout << "\n";

  // ==========================================
  // Result<Void> validation tests
  // ==========================================
  std::cout << "--- Result<Void> Validation ---\n";

  std::cout << "Test 4: validatePositive(5.0) - success\n";
  auto task4 = validatePositive(5.0);
  if (auto err = task4.error())
    printError(*err);
  else
    printSuccess("Validation passed");
  std::cout << "\n";

  std::cout << "Test 5: validatePositive(-3.0) - failure\n";
  auto task5 = validatePositive(-3.0);
  if (auto err = task5.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 6: validateNotEmpty(\"\") - failure\n";
  auto task6 = validateNotEmpty("");
  if (auto err = task6.error())
    printError(*err);
  std::cout << "\n";

  // ==========================================
  // Result<double> using Result<Void> (validation before compute)
  // ==========================================
  std::cout << "--- Result<double> using Result<Void> ---\n";

  std::cout << "Test 7: safeSqrt(16.0) - success\n";
  auto task7 = safeSqrt(16.0);
  if (auto err = task7.error())
    printError(*err);
  else
    printSuccess("sqrt(16) = " + std::to_string(*task7.data()));
  std::cout << "\n";

  std::cout << "Test 8: safeSqrt(-4.0) - validation fails\n";
  auto task8 = safeSqrt(-4.0);
  if (auto err = task8.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 9: parsePositiveDouble(\"3.14\") - success\n";
  auto task9 = parsePositiveDouble("3.14");
  if (auto err = task9.error())
    printError(*err);
  else
    printSuccess("Parsed: " + std::to_string(*task9.data()));
  std::cout << "\n";

  std::cout << "Test 10: parsePositiveDouble(\"-5\") - validation fails after "
               "parse\n";
  auto task10 = parsePositiveDouble("-5");
  if (auto err = task10.error())
    printError(*err);
  std::cout << "\n";

  // ==========================================
  // Result<Void> using Result<double>
  // ==========================================
  std::cout << "--- Result<Void> using Result<double> ---\n";

  std::cout
      << "Test 11: processAndSave(\"10\", \"2\", \"output.txt\") - success\n";
  auto task11 = processAndSave("10", "2", "output.txt");
  if (auto err = task11.error())
    printError(*err);
  else
    printSuccess("Process and save completed");
  std::cout << "\n";

  std::cout << "Test 12: processAndSave(\"10\", \"0\", \"output.txt\") - "
               "division fails\n";
  auto task12 = processAndSave("10", "0", "output.txt");
  if (auto err = task12.error())
    printError(*err);
  std::cout << "\n";

  std::cout
      << "Test 13: processAndSave(\"10\", \"2\", \"\") - empty filename\n";
  auto task13 = processAndSave("10", "2", "");
  if (auto err = task13.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 14: validateComputable(4.0, 2.0) - success\n";
  auto task14 = validateComputable(4.0, 2.0);
  if (auto err = task14.error())
    printError(*err);
  else
    printSuccess("Expression is computable");
  std::cout << "\n";

  std::cout << "Test 15: validateComputable(-1.0, 2.0) - sqrt fails\n";
  auto task15 = validateComputable(-1.0, 2.0);
  if (auto err = task15.error())
    printError(*err);
  std::cout << "\n";

  // ==========================================
  // Complex composition tests
  // ==========================================
  std::cout << "--- Complex Composition ---\n";

  std::cout << "Test 16: computeWeightedAverage({\"10\", \"20\", \"30\"}, {1, "
               "2, 3})\n";
  auto task16 = computeWeightedAverage({"10", "20", "30"}, {1.0, 2.0, 3.0});
  if (auto err = task16.error())
    printError(*err);
  else
    printSuccess("Weighted average: " + std::to_string(*task16.data()));
  std::cout << "\n";

  std::cout << "Test 17: computeWeightedAverage({\"10\", \"bad\"}, {1, 2}) - "
               "parse error\n";
  auto task17 = computeWeightedAverage({"10", "bad"}, {1.0, 2.0});
  if (auto err = task17.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 18: computeWeightedAverage({\"10\", \"20\"}, {1, -2}) - "
               "negative weight\n";
  auto task18 = computeWeightedAverage({"10", "20"}, {1.0, -2.0});
  if (auto err = task18.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 19: batchProcess({{\"10\", \"2\"}, {\"20\", \"4\"}}) - "
               "all succeed\n";
  auto task19 = batchProcess({{"10", "2"}, {"20", "4"}});
  if (auto err = task19.error())
    printError(*err);
  else
    printSuccess("Batch processing completed");
  std::cout << "\n";

  std::cout << "Test 20: batchProcess({{\"10\", \"2\"}, {\"20\", \"0\"}}) - "
               "second fails\n";
  auto task20 = batchProcess({{"10", "2"}, {"20", "0"}});
  if (auto err = task20.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "=== All tests completed ===\n";

  return 0;
}
// NOLINTEND(readability-magic-numbers)
