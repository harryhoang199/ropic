// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <atomic>
#include <climits>
#include <string>

#include "ropic.hpp"

using namespace ropic;

// NOLINTBEGIN(readability-magic-numbers)

// =============================================================================
// Leak Detection Types
// =============================================================================

/// @brief ERROR type that tracks heap allocations to detect memory leaks.
/// Uses atomic counters for thread-safe tracking.
/// Tracks when instances are created via `new` (in Promise) and destroyed.
struct LeakDetectorError
{
  static std::atomic<int> s_instanceCount;

  int code;
  std::string message;

  LeakDetectorError(int c, std::string msg)
      : code(c),
        message(std::move(msg))
  {
    ++s_instanceCount;
  }

  ~LeakDetectorError() { --s_instanceCount; }

  LeakDetectorError(LeakDetectorError&& other) noexcept
      : code(other.code),
        message(std::move(other.message))
  {
    ++s_instanceCount;
    other.code = -1;
  }

  auto operator=(LeakDetectorError&& other) noexcept -> LeakDetectorError&
  {
    if (this != &other)
    {
      code = other.code;
      message = std::move(other.message);
      other.code = -1;
    }
    return *this;
  }

  LeakDetectorError(const LeakDetectorError&) = delete;
  auto operator=(const LeakDetectorError&) -> LeakDetectorError& = delete;

  static void reset() { s_instanceCount = 0; }
  static auto hasLeak() -> bool { return s_instanceCount != 0; }
  static auto count() -> int { return s_instanceCount.load(); }
};

/// @brief Multi-argument constructible ERROR type for tuple return tests.
struct ErrorContext
{
  int code;
  std::string source;
  std::string message;

  ErrorContext(int c, std::string src, std::string msg)
      : code(c),
        source(std::move(src)),
        message(std::move(msg))
  {
  }

  auto operator==(const ErrorContext& other) const -> bool = default;
};

/// @brief Base error class for polymorphic error tests.
struct BaseError
{
  int code;
  std::string message;

  BaseError(int c, std::string msg)
      : code(c),
        message(std::move(msg))
  {
  }
  virtual ~BaseError() = default;

  BaseError(const BaseError&) = default;
  auto operator=(const BaseError&) -> BaseError& = default;
  BaseError(BaseError&&) = default;
  auto operator=(BaseError&&) -> BaseError& = default;

  [[nodiscard]]
  virtual auto describe() const -> std::string
  {
    return message;
  }
};

/// @brief Derived error class for polymorphic error tests.
struct NetworkError : BaseError
{
  std::string endpoint;

  NetworkError(int c, std::string msg, std::string ep)
      : BaseError(c, std::move(msg)),
        endpoint(std::move(ep))
  {
  }

  [[nodiscard]]
  auto describe() const -> std::string override
  {
    return message + " at " + endpoint;
  }
};

/// @brief Derived error class without move/copy constructors for fallback
/// tests. When co_return with this type, it should fallback to BaseError
/// overloads.
struct ImmovableNetworkError : BaseError
{
  std::string endpoint;

  ImmovableNetworkError(int c, std::string msg, std::string ep)
      : BaseError(c, std::move(msg)),
        endpoint(std::move(ep))
  {
  }

  // Delete copy and move constructors to force fallback
  ImmovableNetworkError(const ImmovableNetworkError&) = delete;
  auto operator=(const ImmovableNetworkError&)
      -> ImmovableNetworkError& = delete;
  ImmovableNetworkError(ImmovableNetworkError&&) = delete;
  auto operator=(ImmovableNetworkError&&) -> ImmovableNetworkError& = delete;

  [[nodiscard]]
  auto describe() const -> std::string override
  {
    return message + " [immovable] at " + endpoint;
  }
};

/// @brief Base error class that tracks destructor calls.
struct ErrorDestructorTracker
{
  static int s_destructorCount;
  int code;
  std::string message;

  ErrorDestructorTracker(int c, std::string msg)
      : code(c),
        message(std::move(msg))
  {
  }

  virtual ~ErrorDestructorTracker() { ++s_destructorCount; }

  ErrorDestructorTracker(const ErrorDestructorTracker&) = delete;
  auto operator=(const ErrorDestructorTracker&)
      -> ErrorDestructorTracker& = delete;

  ErrorDestructorTracker(ErrorDestructorTracker&& other) noexcept
      : code(other.code),
        message(std::move(other.message))
  {
    other.code = -1;
  }

  auto operator=(ErrorDestructorTracker&&) -> ErrorDestructorTracker& = delete;

  [[nodiscard]]
  virtual auto describe() const -> std::string
  {
    return message;
  }

  static void reset() { s_destructorCount = 0; }
};

/// @brief Derived error class that tracks destructor calls (polymorphic).
struct DerivedErrorDestructorTracker : ErrorDestructorTracker
{
  static int s_derivedDestructorCount;
  std::string detail;

  DerivedErrorDestructorTracker(int c, std::string msg, std::string det)
      : ErrorDestructorTracker(c, std::move(msg)),
        detail(std::move(det))
  {
  }

  ~DerivedErrorDestructorTracker() override { ++s_derivedDestructorCount; }

  DerivedErrorDestructorTracker(const DerivedErrorDestructorTracker&) = delete;
  auto operator=(const DerivedErrorDestructorTracker&)
      -> DerivedErrorDestructorTracker& = delete;

  DerivedErrorDestructorTracker(DerivedErrorDestructorTracker&& other) noexcept
      : ErrorDestructorTracker(std::move(other)),
        detail(std::move(other.detail))
  {
  }

  auto operator=(DerivedErrorDestructorTracker&&)
      -> DerivedErrorDestructorTracker& = delete;

  [[nodiscard]]
  auto describe() const -> std::string override
  {
    return message + ": " + detail;
  }

  static void resetDerived()
  {
    s_derivedDestructorCount = 0;
    ErrorDestructorTracker::reset();
  }
};

/// @brief ERROR type with deleted move constructor for in-place construction
/// tests.
struct ImmovableError
{
  int code;
  std::string source;
  std::string message;

  ImmovableError(int c, std::string src, std::string msg)
      : code(c),
        source(std::move(src)),
        message(std::move(msg))
  {
  }

  ImmovableError(const ImmovableError&) = delete;
  auto operator=(const ImmovableError&) -> ImmovableError& = delete;
  ImmovableError(ImmovableError&&) = delete;
  auto operator=(ImmovableError&&) -> ImmovableError& = delete;

  auto operator==(const ImmovableError& other) const -> bool = default;
};

struct MoveTracker
{
  static int s_copyCount;
  static int s_moveCount;
  int value;

  explicit MoveTracker(int v)
      : value(v)
  {
  }
  MoveTracker(const MoveTracker& other)
      : value(other.value)
  {
    ++s_copyCount;
  }
  MoveTracker(MoveTracker&& other) noexcept
      : value(other.value)
  {
    ++s_moveCount;
    other.value = -1;
  }
  auto operator=(const MoveTracker& other) -> MoveTracker&
  {
    if (this != &other)
    {
      value = other.value;
      ++s_copyCount;
    }
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

// Deep nesting (5+ levels) - used in P1S04 and P1S06
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

inline auto returnIntWithMoveTrackerError(bool shouldFail)
    -> Either<int, MoveTracker>
{
  if (shouldFail)
    co_return MoveTracker{-1};
  co_return 42;
}

// NOLINTEND(readability-magic-numbers)
