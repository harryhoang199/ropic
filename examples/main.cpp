// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <cassert>
#include <coroutine>
#include <format>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include "Generator.hpp"
#include "examples.h"
#include "tasks.hpp"

#include "ropic/either_state.hpp"

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

  if ((result1.state() != ropic::CoroState::DONE)
      || (result2.state() != ropic::CoroState::DONE))
    co_return kErrorSentinel;

  if (result1.error() || result2.error())
    co_return kErrorSentinel;

  auto data1 = result1.value();
  auto data2 = result2.value();
  if (data1 && data2)
    co_return data1.value() * kMultiplier* data2.value();

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
void testAsyncShallowCoAwait();
void testAsyncNestedCoAwait();
void testFlowOfAsyncCoro();
void testFlowOfAsyncCoroWithSymmetricTransfer();
void testAsyncWithSafeAwaiter();
} // namespace

// ==========================================
// MAIN
// ==========================================

auto main() -> int
{
#ifdef ROPIC_TESTING_MODE
  std::cout << "ROPIC_TESTING_MODE defined\n\n";
#else
  std::cout << "ROPIC_TESTING_MODE NOT defined\n\n";
#endif

  auto exec = [](void (*func)(), bool skip = false)
  {
    if (skip)
      return;
    func();
  };

  std::cout << "=== Testing Railway Oriented Programming ===\n\n";

  exec(testBasicDivision);
  exec(testVoidValidation);
  exec(testDataUsingVoid);
  exec(testVoidUsingData);
  exec(testComplexComposition);
  exec(testTaskIntegration);
  exec(testGeneratorIntegration);
  exec(testAsyncShallowCoAwait, true);
  exec(testAsyncNestedCoAwait, true);
  exec(testFlowOfAsyncCoro, true);
  exec(testFlowOfAsyncCoroWithSymmetricTransfer, true);
  exec(testAsyncWithSafeAwaiter);

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
  assert(task1.state() == ropic::CoroState::DONE);
  if (auto err = task1.error())
    printError(*err);
  else
    printSuccess("Result: " + std::to_string(*task1.value()));
  std::cout << "\n";

  std::cout << "Test 2: divideStr(\".2\", \"0\") - division by zero\n";
  auto task2 = divideStr(".2", "0");
  assert(task2.state() == ropic::CoroState::DONE);
  if (auto err = task2.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 3: divideStr(\"abc\", \"5\") - parse error\n";
  auto task3 = divideStr("abc", "5");
  assert(task3.state() == ropic::CoroState::DONE);
  if (auto err = task3.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 3a: divideStr(\" \\n  \\t \", \"5\") - parse error\n";
  auto task3a = divideStr(" \n  \t ", "5");
  assert(task3a.state() == ropic::CoroState::DONE);
  if (auto err = task3a.error())
    printError(*err);
  std::cout << "\n";
}

void testVoidValidation()
{
  std::cout << "--- Result<Void> Validation ---\n";

  std::cout << "Test 4: validatePositive(5.0) - success\n";
  auto task4 = validatePositive(5.0);
  assert(task4.state() == ropic::CoroState::DONE);
  if (auto err = task4.error())
    printError(*err);
  else
    printSuccess("Validation passed");
  std::cout << "\n";

  std::cout << "Test 5: validatePositive(-3.0) - failure\n";
  auto task5 = validatePositive(-3.0);
  assert(task5.state() == ropic::CoroState::DONE);
  if (auto err = task5.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 6: validateNotEmpty(\"\") - failure\n";
  auto task6 = validateNotEmpty("");
  assert(task6.state() == ropic::CoroState::DONE);
  if (auto err = task6.error())
    printError(*err);
  std::cout << "\n";
}

void testDataUsingVoid()
{
  std::cout << "--- Result<double> using Result<Void> ---\n";

  std::cout << "Test 7: safeSqrt(16.0) - success\n";
  auto task7 = safeSqrt(16.0);
  assert(task7.state() == ropic::CoroState::DONE);
  if (auto err = task7.error())
    printError(*err);
  else
    printSuccess("sqrt(16) = " + std::to_string(*task7.value()));
  std::cout << "\n";

  std::cout << "Test 8: safeSqrt(-4.0) - validation fails\n";
  auto task8 = safeSqrt(-4.0);
  assert(task8.state() == ropic::CoroState::DONE);
  if (auto err = task8.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 9: parsePositiveDouble(\"3.14\") - success\n";
  auto task9 = parsePositiveDouble("3.14");
  assert(task9.state() == ropic::CoroState::DONE);
  if (auto err = task9.error())
    printError(*err);
  else
    printSuccess("Parsed: " + std::to_string(*task9.value()));
  std::cout << "\n";

  std::cout << "Test 10: parsePositiveDouble(\"-5\") - validation fails\n";
  auto task10 = parsePositiveDouble("-5");
  assert(task10.state() == ropic::CoroState::DONE);
  if (auto err = task10.error())
    printError(*err);
  std::cout << "\n";
}

void testVoidUsingData()
{
  std::cout << "--- Result<Void> using Result<double> ---\n";

  std::cout << "Test 11: processAndSave(\"10\", \"2\", \"output.txt\")\n";
  auto task11 = processAndSave("10", "2", "output.txt");
  assert(task11.state() == ropic::CoroState::DONE);
  if (auto err = task11.error())
    printError(*err);
  else
    printSuccess("Process and save completed");
  std::cout << "\n";

  std::cout << "Test 12: processAndSave(\"10\", \"0\", \"output.txt\")\n";
  auto task12 = processAndSave("10", "0", "output.txt");
  assert(task12.state() == ropic::CoroState::DONE);
  if (auto err = task12.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 13: processAndSave(\"10\", \"2\", \"\")\n";
  auto task13 = processAndSave("10", "2", "");
  assert(task13.state() == ropic::CoroState::DONE);
  if (auto err = task13.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 14: validateComputable(4.0, 2.0) - success\n";
  auto task14 = validateComputable(4.0, 2.0);
  assert(task14.state() == ropic::CoroState::DONE);
  if (auto err = task14.error())
    printError(*err);
  else
    printSuccess("Expression is computable");
  std::cout << "\n";

  std::cout << "Test 15: validateComputable(-1.0, 2.0) - sqrt fails\n";
  auto task15 = validateComputable(-1.0, 2.0);
  assert(task15.state() == ropic::CoroState::DONE);
  if (auto err = task15.error())
    printError(*err);
  std::cout << "\n";
}

void testComplexComposition()
{
  std::cout << "--- Complex Composition ---\n";

  std::cout << "Test 16: computeWeightedAverage\n";
  auto task16 = computeWeightedAverage({"10", "20", "30"}, {1.0, 2.0, 3.0});
  assert(task16.state() == ropic::CoroState::DONE);
  if (auto err = task16.error())
    printError(*err);
  else
    printSuccess("Weighted average: " + std::to_string(*task16.value()));
  std::cout << "\n";

  std::cout << "Test 17: computeWeightedAverage - parse error\n";
  auto task17 = computeWeightedAverage({"10", "bad"}, {1.0, 2.0});
  assert(task17.state() == ropic::CoroState::DONE);
  if (auto err = task17.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 18: computeWeightedAverage - negative weight\n";
  auto task18 = computeWeightedAverage({"10", "20"}, {1.0, -2.0});
  assert(task18.state() == ropic::CoroState::DONE);
  if (auto err = task18.error())
    printError(*err);
  std::cout << "\n";

  std::cout << "Test 19: batchProcess - all succeed\n";
  auto task19 = batchProcess({{"10", "2"}, {"20", "4"}});
  assert(task19.state() == ropic::CoroState::DONE);
  if (auto err = task19.error())
    printError(*err);
  else
    printSuccess("Batch processing completed");
  std::cout << "\n";

  std::cout << "Test 20: batchProcess - second fails\n";
  auto task20 = batchProcess({{"10", "2"}, {"20", "0"}});
  assert(task20.state() == ropic::CoroState::DONE);
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
    assert(result.state() == ropic::CoroState::DONE);
    std::cout << "  Item " << idx << ": ";
    if (auto err = result.error())
      printError(*err);
    else
      printSuccess("Result = " + std::to_string(*result.value()));
    ++idx;
  }
  std::cout << "\n";
}

void testAsyncShallowCoAwait()
{
  std::cout << "--- Async Shallow co_await (asyncDivideStr) ---\n";
  std::cout << "Demonstrates single-level co_await on non-Either awaitables\n";
  std::cout << "Each task simulates async fetch (~1s) then divides\n\n";

  // Launch all async tasks into a list
  // Using list allows efficient removal of completed tasks during iteration
  struct TaskEntry
  {
    Result<double> task;
    std::string description;
    std::shared_ptr<std::mutex> mutex;

    TaskEntry(
        Result<double>&& t, std::string desc, std::shared_ptr<std::mutex> mtx)
        : task(std::move(t)),
          description(std::move(desc)),
          mutex(std::move(mtx))
    {
    }
  };

  std::shared_ptr<std::mutex> mutex;
  std::list<TaskEntry> tasks;

  {
    mutex = std::make_shared<std::mutex>();
    std::lock_guard lock(*mutex);
    examples::AsyncFetch fetch(mutex);
    auto task = asyncDivideStr(" 42", "7", std::move(fetch));
    tasks.emplace_back(
        std::move(task),
        R"(Launching: asyncDivideStr(" 42", "7") - success case)",
        mutex);
  }

  {
    mutex = std::make_shared<std::mutex>();
    std::lock_guard lock(*mutex);
    examples::AsyncFetch fetch(mutex);
    auto task = asyncDivideStr("100", "0", std::move(fetch));
    tasks.emplace_back(
        std::move(task),
        R"(Launching: asyncDivideStr("100", "0") - division by zero)",
        mutex);
  }

  {
    mutex = std::make_shared<std::mutex>();
    std::lock_guard lock(*mutex);
    examples::AsyncFetch fetch(mutex);
    auto task = asyncDivideStr("abc", "5", std::move(fetch));
    tasks.emplace_back(
        std::move(task),
        R"(Launching: asyncDivideStr("abc", "5") - parse error)",
        mutex);
  }

  {
    mutex = std::make_shared<std::mutex>();
    std::lock_guard lock(*mutex);
    examples::AsyncFetch fetch(mutex);
    auto task = asyncDivideStr("50", "2", std::move(fetch));
    tasks.emplace_back(
        std::move(task),
        R"(Launching: asyncDivideStr("50", "2") - success case)",
        mutex);
  }

  for (auto& task : tasks)
  {
    std::cout << task.description << "\n";
  }

  std::cout << "\nPolling tasks until all complete...\n\n";

  // Poll loop: check done() on each task, remove when complete
  while (!tasks.empty())
  {
    for (auto it = tasks.begin(); it != tasks.end();)
    {
      bool done;
      {
        std::lock_guard lock(*(it->mutex));
        done = (it->task.state() == ropic::CoroState::DONE);
      }

      if (done)
      {
        std::cout << "Task completed: ";
        if (auto err = it->task.error())
          printError(*err);
        else
          printSuccess("Result = " + std::to_string(*it->task.value()));

        std::cout << "\n";
        it = tasks.erase(it);
      }
      else
      {
        ++it;
      }
    }
  }

  std::cout << "All async tasks completed.\n\n";
}

void testAsyncNestedCoAwait()
{
  std::cout << "--- Async Nested co_await (asyncCrossRatio) ---\n";
  std::cout << "Demonstrates multi-level co_await (asyncCrossRatio calls "
               "asyncDivideStr)\n";
  std::cout << "Each task computes (n1/d1) / (n2/d2) with async fetches\n\n";

  // Launch all async tasks into a list
  // Using list allows efficient removal of completed tasks during iteration
  struct TaskEntry
  {
    Result<double> task;
    std::string description;
    std::shared_ptr<std::mutex> mutex;

    TaskEntry(
        Result<double>&& t, std::string desc, std::shared_ptr<std::mutex> mtx)
        : task(std::move(t)),
          description(std::move(desc)),
          mutex(std::move(mtx))
    {
    }
  };

  std::shared_ptr<std::mutex> mutex;
  std::list<TaskEntry> tasks;

  {
    mutex = std::make_shared<std::mutex>();
    std::lock_guard lock(*mutex);
    examples::AsyncFetch fetch(mutex);
    auto task = asyncCrossRatio(" 10", "2", "20", "4", std::move(fetch));
    tasks.emplace_back(
        std::move(task),
        R"(Launching: asyncCrossRatio(" 10", "2", "20", "4") - success case)",
        mutex);
  }

  {
    mutex = std::make_shared<std::mutex>();
    std::lock_guard lock(*mutex);
    examples::AsyncFetch fetch(mutex);
    auto task = asyncCrossRatio("abc", "2", "20", "4", std::move(fetch));
    tasks.emplace_back(
        std::move(task),
        R"(Launching: asyncCrossRatio("abc", "2", "20", "4") - parse error)",
        mutex);
  }

  {
    mutex = std::make_shared<std::mutex>();
    std::lock_guard lock(*mutex);
    examples::AsyncFetch fetch(mutex);
    auto task = asyncCrossRatio("10 ", "2", ".0", "4", std::move(fetch));
    tasks.emplace_back(
        std::move(task),
        R"(Launching: asyncCrossRatio("10 ", "2", ".0", "4") - div by zero)",
        mutex);
  }

  for (auto& task : tasks)
  {
    std::cout << task.description << "\n";
  }

  std::cout << "\nPolling tasks until all complete...\n\n";

  // Poll loop: check done() on each task, remove when complete
  while (!tasks.empty())
  {
    for (auto it = tasks.begin(); it != tasks.end();)
    {
      bool done;
      {
        std::lock_guard lock(*(it->mutex));
        done = (it->task.state() == ropic::CoroState::DONE);
      }

      if (!done)
      {
        ++it;
        continue;
      }

      std::cout << "Task completed: ";
      if (auto err = it->task.error())
        printError(*err);
      else
        printSuccess("Result = " + std::to_string(*it->task.value()));

      std::cout << "\n";
      it = tasks.erase(it);
    }
  }

  std::cout << "All async tasks completed.\n\n";
}

void testFlowOfAsyncCoro()
{
  std::cout
      << "--- Flow of Coroutine Execution ---\n"
      << R"(Launching: asyncCrossRatio(" 10", "2", "20", "4") - success case)"
      << "\n";

  std::coroutine_handle<> suspendedHandle = nullptr;
  std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
  examples::AsyncFetchExposingHandle fetch(mutex, &suspendedHandle);
  auto task = asyncCrossRatio(" 10", "2", "20", "4", std::move(fetch));

  while (true)
  {
    std::lock_guard lock(*mutex);
    if (suspendedHandle)
    {
      suspendedHandle.resume();
      suspendedHandle = nullptr;
    }
    if (task.state() == ropic::CoroState::DONE)
      break;
  }

  std::cout << "Task completed: ";
  if (auto err = task.error())
    printError(*err);
  else
    printSuccess("Result = " + std::to_string(*task.value()));
}

void testFlowOfAsyncCoroWithSymmetricTransfer()
{
  std::cout
      << "--- Flow of Coroutine Execution With Symmetric Transfer ---\n"
      << R"(Launching: asyncCrossRatio(" 10", "2", "20", "4") - success case)"
      << "\n";

  auto runTask =
      [](std::string n1, std::string d1, std::string n2, std::string d2)
  {
    auto mutex = std::make_shared<std::mutex>();
    std::coroutine_handle<> suspendedHandle = nullptr;
    auto simpleTask = []() -> examples::SimpleTask<int>
    {
      co_await std::suspend_always{};
      co_return 1;
    }();

    examples::AsyncFetchWithSymmetricTransfer fetch(
        mutex, &suspendedHandle, simpleTask.handle);
    auto eitherTask = asyncCrossRatio(
        std::move(n1),
        std::move(d1),
        std::move(n2),
        std::move(d2),
        std::move(fetch));

    while (true)
    {
      std::lock_guard lock(*mutex);
      if (suspendedHandle)
      {
        suspendedHandle.resume();
        suspendedHandle = nullptr;
      }
      if (eitherTask.state() == ropic::CoroState::DONE)
        break;
    }

    printSuccess("Task completed: " + std::to_string(*(eitherTask.value())));
  };

  // ===== Task 1 =====
  runTask(" 10", "2", "20", "4");

  // ===== Task 2 =====
  runTask(".35", "5 ", "  -.7", "2");
}

void testAsyncWithSafeAwaiter()
{
  std::cout << "--- Async with Safe Awaiter (asyncCrossRatio) ---\n";
  std::cout
      << "Demonstrates multi-level co_await with SafeAwaiter (no need mutex)\n";
  std::cout << "Each task computes (n1/d1) / (n2/d2) with SafeAwaiter\n\n";

  // Launch all async tasks into a list
  // Using list allows efficient removal of completed tasks during iteration
  struct TaskEntry
  {
    Result<double> task;
    std::string description;

    TaskEntry(Result<double>&& t, std::string desc)
        : task(std::move(t)),
          description(std::move(desc))
    {
    }
  };

  std::list<TaskEntry> tasks;

  using ResumeMode = examples::SafeAsyncFetch::ResumeMode;

  struct Input
  {
    std::string n1, d1, n2, d2, label;
  };

  std::vector<Input> inputs{
      {.n1 = " 10", .d1 = "2", .n2 = "20", .d2 = "4", .label = "success case"},
      {.n1 = "abc", .d1 = "2", .n2 = "20", .d2 = "4", .label = "parse error"},
      {.n1 = "10 ", .d1 = "2", .n2 = ".0", .d2 = "4", .label = "div by zero"},
  };

  std::vector<std::pair<ResumeMode, std::string>> modes{
      {ResumeMode::INLINE, "INLINE"},
      {ResumeMode::ASYNC, "ASYNC"},
      {ResumeMode::ASYNC_DELAYED, "ASYNC_DELAYED"},
  };

  for (auto& [mode, modeName] : modes)
  {
    for (auto& in : inputs)
    {
      examples::SafeAsyncFetch fetch(mode);
      auto task = asyncCrossRatio(in.n1, in.d1, in.n2, in.d2, std::move(fetch));
      tasks.emplace_back(
          std::move(task),
          std::format(
              R"(asyncCrossRatio("{}", "{}", "{}", "{}") [{}] - {})",
              in.n1,
              in.d1,
              in.n2,
              in.d2,
              modeName,
              in.label));
    }
  }

  for (auto& task : tasks)
  {
    std::cout << task.description << "\n";
  }

  std::cout << "\nPolling tasks until all complete...\n\n";

  // Poll loop: check done() on each task, remove when complete
  while (!tasks.empty())
  {
    for (auto it = tasks.begin(); it != tasks.end();)
    {
      switch (it->task.state())
      {
      case ropic::CoroState::DONE:
      {
        std::cout << "Task completed: ";
        if (auto err = it->task.error())
          printError(*err);
        else
          printSuccess("Result = " + std::to_string(*it->task.value()));

        std::cout << "\n";
        it = tasks.erase(it);
        break;
      }
      case ropic::CoroState::READY:
      {
        it->task.resume();
        ++it;
        break;
      }
      case ropic::CoroState::PENDING:
      {
        ++it;
        break;
      }
      default:
      {
        assert(false && "This case should never happen");
        break;
      }
      }
    }
  }

  std::cout << "All async tasks completed.\n\n";
}

// NOLINTEND(readability-magic-numbers)

} // namespace
