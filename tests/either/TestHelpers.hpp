// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <array>
#include <climits>
#include <string>

#include "ropic.hpp"

using namespace ropic;

// =============================================================================
// Test Helper Types
// =============================================================================

// NOLINTBEGIN(readability-magic-numbers)
struct TestData
{
  int value;
  std::string name;
  bool operator==(const TestData& other) const = default;
};

struct TestError
{
  int code;
  std::string message;
  bool operator==(const TestError& other) const = default;
};

struct MoveTracker
{
  static int s_copyCount;
  static int s_moveCount;
  int value;

  explicit MoveTracker(int v) : value(v) {}
  MoveTracker(const MoveTracker& other) : value(other.value) { ++s_copyCount; }
  MoveTracker(MoveTracker&& other) noexcept : value(other.value)
  {
    ++s_moveCount;
    other.value = -1;
  }
  auto operator=(const MoveTracker& other) -> MoveTracker&
  {
    value = other.value;
    ++s_copyCount;
    return *this;
  }
  auto operator=(MoveTracker&& other) noexcept -> MoveTracker&
  {
    value = other.value;
    ++s_moveCount;
    other.value = -1;
    return *this;
  }
  static void reset()
  {
    s_copyCount = 0;
    s_moveCount = 0;
  }
  auto operator==(const MoveTracker& other) const -> bool
  {
    return value == other.value;
  }
};

struct LargeStruct
{
  std::array<int, 100> values;
  std::string name;
  bool operator==(const LargeStruct& other) const = default;
};

// =============================================================================
// Helper Coroutines
// =============================================================================

inline auto returnData(int x) -> Either<int, std::string> { co_return x; }
inline auto returnError(std::string msg) -> Either<int, std::string>
{
  co_return msg;
}
inline auto returnOK() -> Either<Void, std::string> { co_return OK; }
inline auto returnVoidError(std::string msg) -> Either<Void, std::string>
{
  co_return msg;
}

inline auto awaitAndAdd(Either<int, std::string> input, int delta)
    -> Either<int, std::string>
{
  int val = co_await std::move(input);
  co_return val + delta;
}

inline auto chainedAwaitsAllSucceed(int start) -> Either<int, std::string>
{
  int a = co_await returnData(start);
  int b = co_await returnData(a + 10);
  int c = co_await returnData(b + 100);
  co_return c;
}

inline auto chainedAwaitsFirstFails() -> Either<int, std::string>
{
  int a = co_await returnError("first failed");
  int b = co_await returnData(a + 10);
  co_return b;
}

inline auto chainedAwaitsMiddleFails(int start) -> Either<int, std::string>
{
  [[maybe_unused]]
  int a = co_await returnData(start);
  int b = co_await returnError("middle failed");
  co_return b + 100;
}

inline auto innerSuccess(int x) -> Either<int, std::string>
{
  co_return x * 2;
}
inline auto innerError() -> Either<int, std::string>
{
  co_return std::string("inner error");
}

inline auto outerCallsInnerSuccess(int x) -> Either<int, std::string>
{
  int result = co_await innerSuccess(x);
  co_return result + 5;
}

inline auto outerCallsInnerError() -> Either<int, std::string>
{
  int result = co_await innerError();
  co_return result + 5;
}

inline auto mixedTypeCoroutine(int x) -> Either<double, std::string>
{
  int val = co_await returnData(x);
  co_return static_cast<double>(val) * 1.5;
}

inline auto validatePositive(int x) -> Either<Void, std::string>
{
  if (x <= 0)
    co_return std::string("must be positive");
  co_return OK;
}

inline auto computeWithValidation(int x) -> Either<int, std::string>
{
  co_await validatePositive(x);
  co_return x * 2;
}

// Deep nesting (5+ levels)
inline auto level5(int x) -> Either<int, std::string> { co_return x + 1; }
inline auto level4(int x) -> Either<int, std::string>
{
  int v = co_await level5(x);
  co_return v + 1;
}
inline auto level3(int x) -> Either<int, std::string>
{
  int v = co_await level4(x);
  co_return v + 1;
}
inline auto level2(int x) -> Either<int, std::string>
{
  int v = co_await level3(x);
  co_return v + 1;
}
inline auto level1(int x) -> Either<int, std::string>
{
  int v = co_await level2(x);
  co_return v + 1;
}

inline auto level5Error() -> Either<int, std::string>
{
  co_return std::string("deep error");
}
inline auto level4Error() -> Either<int, std::string>
{
  int v = co_await level5Error();
  co_return v + 1;
}
inline auto level3Error() -> Either<int, std::string>
{
  int v = co_await level4Error();
  co_return v + 1;
}
inline auto level2Error() -> Either<int, std::string>
{
  int v = co_await level3Error();
  co_return v + 1;
}
inline auto level1Error() -> Either<int, std::string>
{
  int v = co_await level2Error();
  co_return v + 1;
}

inline auto returnMoveTracker(int x) -> Either<MoveTracker, std::string>
{
  co_return MoveTracker{x};
}

inline auto awaitMoveTracker(int x) -> Either<MoveTracker, std::string>
{
  MoveTracker val = co_await returnMoveTracker(x);
  co_return MoveTracker{val.value + 10};
}

inline auto returnIntWithMoveTrackerError(bool shouldFail)
    -> Either<int, MoveTracker>
{
  if (shouldFail)
    co_return MoveTracker{-1};
  co_return 42;
}
// NOLINTEND(readability-magic-numbers)
