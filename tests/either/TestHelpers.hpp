// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "ropic.hpp"
#include <array>
#include <climits>
#include <string>

using namespace ropic;

// =============================================================================
// Test Helper Types
// =============================================================================

struct TestData
{
  int value;
  std::string name;
  bool operator==(const TestData &other) const = default;
};

struct TestError
{
  int code;
  std::string message;
  bool operator==(const TestError &other) const = default;
};

struct MoveTracker
{
  static int copyCount;
  static int moveCount;
  int value;

  explicit MoveTracker(int v) : value(v) {}
  MoveTracker(const MoveTracker &other) : value(other.value) { ++copyCount; }
  MoveTracker(MoveTracker &&other) noexcept : value(other.value)
  {
    ++moveCount;
    other.value = -1;
  }
  MoveTracker &operator=(const MoveTracker &other)
  {
    value = other.value;
    ++copyCount;
    return *this;
  }
  MoveTracker &operator=(MoveTracker &&other) noexcept
  {
    value = other.value;
    ++moveCount;
    other.value = -1;
    return *this;
  }
  static void reset()
  {
    copyCount = 0;
    moveCount = 0;
  }
  bool operator==(const MoveTracker &other) const { return value == other.value; }
};

struct LargeStruct
{
  std::array<int, 100> values;
  std::string name;
  bool operator==(const LargeStruct &other) const = default;
};

// =============================================================================
// Helper Coroutines
// =============================================================================

inline Either<int, std::string> returnData(int x) { co_return x; }
inline Either<int, std::string> returnError(std::string msg) { co_return msg; }
inline Either<Void, std::string> returnOK() { co_return OK; }
inline Either<Void, std::string> returnVoidError(std::string msg) { co_return msg; }

inline Either<int, std::string> awaitAndAdd(Either<int, std::string> input, int delta)
{
  int val = co_await std::move(input);
  co_return val + delta;
}

inline Either<int, std::string> chainedAwaitsAllSucceed(int start)
{
  int a = co_await returnData(start);
  int b = co_await returnData(a + 10);
  int c = co_await returnData(b + 100);
  co_return c;
}

inline Either<int, std::string> chainedAwaitsFirstFails()
{
  int a = co_await returnError("first failed");
  int b = co_await returnData(a + 10);
  co_return b;
}

inline Either<int, std::string> chainedAwaitsMiddleFails(int start)
{
  [[maybe_unused]] int a = co_await returnData(start);
  int b = co_await returnError("middle failed");
  co_return b + 100;
}

inline Either<int, std::string> innerSuccess(int x) { co_return x * 2; }
inline Either<int, std::string> innerError() { co_return std::string("inner error"); }

inline Either<int, std::string> outerCallsInnerSuccess(int x)
{
  int result = co_await innerSuccess(x);
  co_return result + 5;
}

inline Either<int, std::string> outerCallsInnerError()
{
  int result = co_await innerError();
  co_return result + 5;
}

inline Either<double, std::string> mixedTypeCoroutine(int x)
{
  int val = co_await returnData(x);
  co_return static_cast<double>(val) * 1.5;
}

inline Either<Void, std::string> validatePositive(int x)
{
  if (x <= 0)
    co_return std::string("must be positive");
  co_return OK;
}

inline Either<int, std::string> computeWithValidation(int x)
{
  co_await validatePositive(x);
  co_return x * 2;
}

// Deep nesting (5+ levels)
inline Either<int, std::string> level5(int x) { co_return x + 1; }
inline Either<int, std::string> level4(int x)
{
  int v = co_await level5(x);
  co_return v + 1;
}
inline Either<int, std::string> level3(int x)
{
  int v = co_await level4(x);
  co_return v + 1;
}
inline Either<int, std::string> level2(int x)
{
  int v = co_await level3(x);
  co_return v + 1;
}
inline Either<int, std::string> level1(int x)
{
  int v = co_await level2(x);
  co_return v + 1;
}

inline Either<int, std::string> level5Error() { co_return std::string("deep error"); }
inline Either<int, std::string> level4Error()
{
  int v = co_await level5Error();
  co_return v + 1;
}
inline Either<int, std::string> level3Error()
{
  int v = co_await level4Error();
  co_return v + 1;
}
inline Either<int, std::string> level2Error()
{
  int v = co_await level3Error();
  co_return v + 1;
}
inline Either<int, std::string> level1Error()
{
  int v = co_await level2Error();
  co_return v + 1;
}

inline Either<MoveTracker, std::string> returnMoveTracker(int x) { co_return MoveTracker{x}; }

inline Either<MoveTracker, std::string> awaitMoveTracker(int x)
{
  MoveTracker val = co_await returnMoveTracker(x);
  co_return MoveTracker{val.value + 10};
}

inline Either<int, MoveTracker> returnIntWithMoveTrackerError(bool shouldFail)
{
  if (shouldFail)
    co_return MoveTracker{-1};
  co_return 42;
}
