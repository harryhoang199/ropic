// Railway Oriented Programming with C++20 Coroutines
//
// File organization:
//   - src/error.h     : Error types (ErrorTag, Error) and utilities
//   - src/either.h    : Either<ERROR, DATA> coroutine template for ROP
//   - src/examples.h  : Example coroutine functions demonstrating usage
//   - Task.hpp        : Task coroutine type for async integration
//   - Generator.hpp   : Generator coroutine type for streaming integration
//   - main.cpp        : Test cases and main function (this file)

#include <cassert>
#include <iostream>
#include <list>
#include <vector>

#include "examples.h"
#include "tasks.hpp"

#include "Generator.hpp"

namespace
{
// ==========================================
// OUTPUT HELPERS
// ==========================================

void printSuccess(const std::string& msg)
{
  std::cout << "[OK] " << msg << "\n";
}

void printError(const Error& err)
{
  std::cout
      << "[FAIL] "
      << err.message()
      << " (tag: "
      << toString(err.tag())
      << ")\n";
}

// ==========================================
// INTEGRATION EXAMPLES
// ==========================================

// Example: Task coroutine that calls Either-returning coroutines
// Note: Takes strings by value since Task is lazy (starts suspended)
// and references would become dangling before coroutine resumes.

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
auto computeInTask(std::string a, std::string b) -> examples::SimpleTask<double>
{
  constexpr double kMultiplier = 500.0;
  constexpr double kErrorSentinel = -1.0;

  auto result1 = co_await divideStr(a, b);

  auto task2 = divideStr(b, a);
  auto& result2 = co_await task2; // result2 is a reference of task2

  if (!result1.done() || !result2.done())
    co_return kErrorSentinel;

  if (result1.error() || result2.error())
    co_return kErrorSentinel;

  auto data1 = result1.data();
  auto data2 = result2.data();
  if (data1 && data2)
    co_return data1.value() * kMultiplier * data2.value();

  co_return kErrorSentinel;
}

// Example: Generator that yields Either values from a batch of operations
auto generateResults(
    const std::vector<std::pair<std::string, std::string>>& inputs)
    -> examples::Generator<Result<double>>
{
  for (const auto& [num, den] : inputs)
  {
    co_yield co_await divideStr(num, den);
  }
}

// ==========================================
// TEST FUNCTION DECLARATIONS
// ==========================================

void testBasicDivision();
void testVoidValidation();
void testDataUsingVoid();
void testVoidUsingData();
void testComplexComposition();
void testTaskIntegration();
void testGeneratorIntegration();
void testAsyncEitherIntegration();

} // namespace

// ==========================================
// MAIN
// ==========================================

auto main() -> int
{
  std::cout << "=== Testing Railway Oriented Programming ===\n\n";

  testBasicDivision();
  testVoidValidation();
  testDataUsingVoid();
  testVoidUsingData();
  testComplexComposition();
  testTaskIntegration();
  testGeneratorIntegration();
  testAsyncEitherIntegration();

  std::cout << "=== All tests completed ===\n";

  return 0;
}

// ==========================================
// TEST FUNCTION DEFINITIONS
// ==========================================

namespace
{
// NOLINTBEGIN(readability-magic-numbers)

void testBasicDivision()
{
  std::cout << "--- Basic Result<double> ---\n";

  std::cout << "Test 1: divideStr(\"10.2\", \"5\") - success case\n";
  auto task1 = divideStr("10.2", "5");
  assert(task1.done());
  if (auto err = task1.error())
    printError(*err);
  else
    printSuccess("Result: " + std::to_string(*task1.data()));
  std::cout << "\n";

  std::cout << "Test 2: divideStr(\".2\", \"0\") - division by zero\n";
  auto task2 = divideStr(".2", "0");
  assert(task2.done());
  if (auto err = task2.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 3: divideStr(\"abc\", \"5\") - parse error\n";
  auto task3 = divideStr("abc", "5");
  assert(task3.done());
  if (auto err = task3.error())
    printError(*err);
  std::cout << "\n";
}

void testVoidValidation()
{
  std::cout << "--- Result<Void> Validation ---\n";

  std::cout << "Test 4: validatePositive(5.0) - success\n";
  auto task4 = validatePositive(5.0);
  assert(task4.done());
  if (auto err = task4.error())
    printError(*err);
  else
    printSuccess("Validation passed");
  std::cout << "\n";

  std::cout << "Test 5: validatePositive(-3.0) - failure\n";
  auto task5 = validatePositive(-3.0);
  assert(task5.done());
  if (auto err = task5.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 6: validateNotEmpty(\"\") - failure\n";
  auto task6 = validateNotEmpty("");
  assert(task6.done());
  if (auto err = task6.error())
    printError(*err);
  std::cout << "\n";
}

void testDataUsingVoid()
{
  std::cout << "--- Result<double> using Result<Void> ---\n";

  std::cout << "Test 7: safeSqrt(16.0) - success\n";
  auto task7 = safeSqrt(16.0);
  assert(task7.done());
  if (auto err = task7.error())
    printError(*err);
  else
    printSuccess("sqrt(16) = " + std::to_string(*task7.data()));
  std::cout << "\n";

  std::cout << "Test 8: safeSqrt(-4.0) - validation fails\n";
  auto task8 = safeSqrt(-4.0);
  assert(task8.done());
  if (auto err = task8.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 9: parsePositiveDouble(\"3.14\") - success\n";
  auto task9 = parsePositiveDouble("3.14");
  assert(task9.done());
  if (auto err = task9.error())
    printError(*err);
  else
    printSuccess("Parsed: " + std::to_string(*task9.data()));
  std::cout << "\n";

  std::cout << "Test 10: parsePositiveDouble(\"-5\") - validation fails\n";
  auto task10 = parsePositiveDouble("-5");
  assert(task10.done());
  if (auto err = task10.error())
    printError(*err);
  std::cout << "\n";
}

void testVoidUsingData()
{
  std::cout << "--- Result<Void> using Result<double> ---\n";

  std::cout << "Test 11: processAndSave(\"10\", \"2\", \"output.txt\")\n";
  auto task11 = processAndSave("10", "2", "output.txt");
  assert(task11.done());
  if (auto err = task11.error())
    printError(*err);
  else
    printSuccess("Process and save completed");
  std::cout << "\n";

  std::cout << "Test 12: processAndSave(\"10\", \"0\", \"output.txt\")\n";
  auto task12 = processAndSave("10", "0", "output.txt");
  assert(task12.done());
  if (auto err = task12.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 13: processAndSave(\"10\", \"2\", \"\")\n";
  auto task13 = processAndSave("10", "2", "");
  assert(task13.done());
  if (auto err = task13.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 14: validateComputable(4.0, 2.0) - success\n";
  auto task14 = validateComputable(4.0, 2.0);
  assert(task14.done());
  if (auto err = task14.error())
    printError(*err);
  else
    printSuccess("Expression is computable");
  std::cout << "\n";

  std::cout << "Test 15: validateComputable(-1.0, 2.0) - sqrt fails\n";
  auto task15 = validateComputable(-1.0, 2.0);
  assert(task15.done());
  if (auto err = task15.error())
    printError(*err);
  std::cout << "\n";
}

void testComplexComposition()
{
  std::cout << "--- Complex Composition ---\n";

  std::cout << "Test 16: computeWeightedAverage\n";
  auto task16 = computeWeightedAverage({"10", "20", "30"}, {1.0, 2.0, 3.0});
  assert(task16.done());
  if (auto err = task16.error())
    printError(*err);
  else
    printSuccess("Weighted average: " + std::to_string(*task16.data()));
  std::cout << "\n";

  std::cout << "Test 17: computeWeightedAverage - parse error\n";
  auto task17 = computeWeightedAverage({"10", "bad"}, {1.0, 2.0});
  assert(task17.done());
  if (auto err = task17.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 18: computeWeightedAverage - negative weight\n";
  auto task18 = computeWeightedAverage({"10", "20"}, {1.0, -2.0});
  assert(task18.done());
  if (auto err = task18.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 19: batchProcess - all succeed\n";
  auto task19 = batchProcess({{"10", "2"}, {"20", "4"}});
  assert(task19.done());
  if (auto err = task19.error())
    printError(*err);
  else
    printSuccess("Batch processing completed");
  std::cout << "\n";

  std::cout << "Test 20: batchProcess - second fails\n";
  auto task20 = batchProcess({{"10", "2"}, {"20", "0"}});
  assert(task20.done());
  if (auto err = task20.error())
    printError(*err);
  std::cout << "\n";
}

void testTaskIntegration()
{
  std::cout << "--- Task Coroutine Integration ---\n";
  std::cout << "Demonstrates calling Either-returning coroutines from Task\n\n";

  std::cout << "Test 21: Task calling divideStr(\"10\", \"2\") - success\n";
  auto task21 = computeInTask("10", "2");
  double result21 = task21.run();
  if (result21 >= 0)
    printSuccess("Task result: " + std::to_string(result21));
  else
    std::cout << "[INFO] Task detected error from Either\n";
  std::cout << "\n";

  std::cout << "Test 22: Task calling divideStr(\"10\", \"0\") - error\n";
  auto task22 = computeInTask("10", "0");
  double result22 = task22.run();
  if (result22 >= 0)
    printSuccess("Task result: " + std::to_string(result22));
  else
    std::cout << "[INFO] Task detected error from Either\n";
  std::cout << "\n";
}

void testGeneratorIntegration()
{
  std::cout << "--- Generator Coroutine Integration ---\n";
  std::cout << "Demonstrates Generator yielding Either values\n\n";

  std::vector<std::pair<std::string, std::string>> inputs = {
      {"10", "2"}, {"20", "4"}, {"15", "0"}, {"8", "2"}};

  std::cout << "Test 23: Generator yielding Results from batch operations\n";
  auto gen = generateResults(inputs);
  int idx = 0;
  while (gen.next())
  {
    auto& result = gen.value();
    assert(result.done());
    std::cout << "  Item " << idx << ": ";
    if (auto err = result.error())
      printError(*err);
    else
      printSuccess("Result = " + std::to_string(*result.data()));
    ++idx;
  }
  std::cout << "\n";
}

void testAsyncEitherIntegration()
{
  std::cout << "--- Async Either Integration ---\n";
  std::cout << "Demonstrates co_await on non-Either awaitables within Either\n";
  std::cout << "Each task simulates async fetch (~1s) then divides\n\n";

  // Launch all async tasks into a list
  // Using list allows efficient removal of completed tasks during iteration
  std::list<Result<double>> tasks;

  std::cout << "Launching: asyncDivideStr(\" 42\", \"7\") - success case\n";
  tasks.push_back(asyncDivideStr(" 42", "7"));

  std::cout << "Launching: asyncDivideStr(\"100\", \"0\") - division by zero\n";
  tasks.push_back(asyncDivideStr("100", "0"));

  std::cout << "Launching: asyncDivideStr(\"abc\", \"5\") - parse error\n";
  tasks.push_back(asyncDivideStr("abc", "5"));

  std::cout << "Launching: asyncDivideStr(\"50\", \"2\") - success case\n";
  tasks.push_back(asyncDivideStr("50", "2"));

  std::cout << "\nPolling tasks until all complete...\n\n";

  // Poll loop: check done() on each task, remove when complete
  while (!tasks.empty())
  {
    for (auto it = tasks.begin(); it != tasks.end();)
    {
      if (it->done())
      {
        std::cout << "Task completed: ";
        if (auto err = it->error())
          printError(*err);
        else
          printSuccess("Result = " + std::to_string(*it->data()));

        std::cout << "\n";
        it = tasks.erase(it);
      }
      else
      {
        ++it;
      }
    }

    if (!tasks.empty())
    {
      constexpr int kPollIntervalMs = 100;
      std::this_thread::sleep_for(std::chrono::milliseconds(kPollIntervalMs));
    }
  }

  std::cout << "All async tasks completed.\n\n";
}

// NOLINTEND(readability-magic-numbers)

} // namespace
