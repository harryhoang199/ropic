// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <atomic>
#include <chrono>
#include <coroutine>
#include <gtest/gtest.h>
#include <memory>
#include <ropic.hpp>
#include <thread>

// =============================================================================
// Custom Awaiters for Testing Move Semantics
// =============================================================================
using namespace ropic;

namespace
{ // NOLINTBEGIN(readability-magic-numbers, readability-identifier-naming)

/// @brief A simple awaiter that suspends and stores the coroutine handle.
/// Used to test move semantics with suspended coroutines.
struct ManualResumeAwaiter
{
  std::coroutine_handle<> handle = nullptr;

  [[nodiscard]]
  static auto await_ready() noexcept -> bool
  {
    return false;
  }

  auto await_suspend(std::coroutine_handle<> h) noexcept -> bool
  {
    handle = h;
    return true;
  }

  static void await_resume() noexcept {}

  void resume() const
  {
    if (handle)
      handle.resume();
  }
};

/// @brief An awaiter that returns a value after manual resume.
template <typename T>
struct ManualResumeAwaiterWithValue
{
  std::coroutine_handle<> handle = nullptr;
  T value;

  explicit ManualResumeAwaiterWithValue(T v) : value(std::move(v)) {}

  [[nodiscard]]
  static auto await_ready() noexcept -> bool
  {
    return false;
  }

  auto await_suspend(std::coroutine_handle<> h) noexcept -> bool
  {
    handle = h;
    return true;
  }

  [[nodiscard]]
  auto await_resume() noexcept -> T
  {
    return std::move(value);
  }

  void resume()
  {
    if (handle)
      handle.resume();
  }
};

/// @brief An async awaiter that resumes after a delay in a background thread.
/// Uses a shared_ptr to ensure proper cleanup even with detached threads.
struct AsyncDelayAwaiter
{
  std::chrono::milliseconds delay;
  std::shared_ptr<std::atomic<bool>> completed =
      std::make_shared<std::atomic<bool>>(false);

  explicit AsyncDelayAwaiter(std::chrono::milliseconds d) : delay(d) {}

  [[nodiscard]]
  static auto await_ready() noexcept -> bool
  {
    return false;
  }

  void await_suspend(std::coroutine_handle<> h) const
  {
    auto flag = completed;
    std::thread(
        [h, d = delay, flag]
        {
          std::this_thread::sleep_for(d);
          h.resume();
          flag->store(true);
        })
        .detach();
  }

  static void await_resume() noexcept {}
};

/// @brief Helper to poll for completion with timeout.
template <typename F>
auto pollUntilDone(F&& isDone, std::chrono::milliseconds timeout) -> bool
{
  auto start = std::chrono::steady_clock::now();
  while (!isDone())
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    if (std::chrono::steady_clock::now() - start > timeout)
      return false;
  }
  return true;
}
} // namespace

// =============================================================================
// Test Suite: Move Semantics with Promise Reference Verification
// =============================================================================

TEST(EitherMovePromiseReference, UNIT_049_MoveWhileSuspendedThenResume)
{
  RecordProperty("id", "0.02-UNIT-049");
  RecordProperty(
      "desc",
      "Move Either while suspended, then resume - promise correctly references "
      "new Either");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 42;
  };

  auto originalEither = coro();

  EXPECT_FALSE(originalEither.done());
  EXPECT_FALSE(originalEither.data());
  EXPECT_FALSE(originalEither.error());

  // Move to new location
  auto movedEither = std::move(originalEither);

  // Verify original is moved-from (empty state)
  EXPECT_FALSE(originalEither.done())
      << "Moved-from Either should report not done (monostate)";
  EXPECT_FALSE(originalEither.data())
      << "Moved-from Either should have no data";
  EXPECT_FALSE(originalEither.error())
      << "Moved-from Either should have no error";

  // Verify moved-to is still not done (suspended)
  EXPECT_FALSE(movedEither.done()) << "Moved Either should still be suspended";

  // Resume - this should store result in movedEither, not originalEither
  awaiter.resume();

  // Verify moved-to Either received the result
  EXPECT_TRUE(movedEither.done()) << "Moved Either should be done after resume";
  EXPECT_TRUE(movedEither.data())
      << "Moved Either should have data after resume";
  EXPECT_EQ(*movedEither.data(), 42);

  // Verify original is still empty
  EXPECT_FALSE(originalEither.done());
  EXPECT_FALSE(originalEither.data());
}

TEST(EitherMovePromiseReference, UNIT_050_MoveAssignWhileSuspendedThenResume)
{
  RecordProperty("id", "0.02-UNIT-050");
  RecordProperty(
      "desc",
      "Move-assign Either while suspended, then resume - promise correctly "
      "references new Either");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 99;
  };

  auto originalEither = coro();
  Either<int, std::string> targetEither{0};

  EXPECT_FALSE(originalEither.done());
  EXPECT_TRUE(targetEither.done());

  // Move-assign to existing Either
  targetEither = std::move(originalEither);

  EXPECT_FALSE(originalEither.done());
  EXPECT_FALSE(targetEither.done());

  // Resume
  awaiter.resume();

  EXPECT_TRUE(targetEither.done());
  EXPECT_TRUE(targetEither.data());
  EXPECT_EQ(*targetEither.data(), 99);

  EXPECT_FALSE(originalEither.done());
}

TEST(EitherMovePromiseReference, UNIT_051_MoveWhileSuspendedErrorPath)
{
  RecordProperty("id", "0.02-UNIT-051");
  RecordProperty(
      "desc",
      "Move Either while suspended, resume with error - promise correctly "
      "references new Either");

  ManualResumeAwaiterWithValue<bool> awaiter{true};

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    bool shouldError = co_await awaiter;
    if (shouldError)
      co_return std::string("error after move");
    co_return 123;
  };

  auto originalEither = coro();

  EXPECT_FALSE(originalEither.done());

  auto movedEither = std::move(originalEither);

  EXPECT_FALSE(movedEither.done());

  awaiter.resume();

  EXPECT_TRUE(movedEither.done());
  EXPECT_TRUE(movedEither.error());
  EXPECT_EQ(*movedEither.error(), "error after move");

  EXPECT_FALSE(originalEither.done());
  EXPECT_FALSE(originalEither.error());
}

TEST(EitherMovePromiseReference, UNIT_052_MultipleMovesThenResume)
{
  RecordProperty("id", "0.02-UNIT-052");
  RecordProperty(
      "desc",
      "Move Either multiple times while suspended - promise tracks final "
      "location");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 777;
  };

  auto either1 = coro();
  EXPECT_FALSE(either1.done());

  auto either2 = std::move(either1);
  EXPECT_FALSE(either1.done());
  EXPECT_FALSE(either2.done());

  auto either3 = std::move(either2);
  EXPECT_FALSE(either2.done());
  EXPECT_FALSE(either3.done());

  Either<int, std::string> either4{0};
  either4 = std::move(either3);
  EXPECT_FALSE(either3.done());
  EXPECT_FALSE(either4.done());

  awaiter.resume();

  // Only the final location should have the result
  EXPECT_TRUE(either4.done());
  EXPECT_TRUE(either4.data());
  EXPECT_EQ(*either4.data(), 777);

  // All others should be empty
  EXPECT_FALSE(either1.done());
  EXPECT_FALSE(either2.done());
  EXPECT_FALSE(either3.done());
}

TEST(EitherMovePromiseReference, UNIT_053_MoveVoidEitherWhileSuspended)
{
  RecordProperty("id", "0.02-UNIT-053");
  RecordProperty(
      "desc",
      "Move Either<Void, Error> while suspended - promise correctly references "
      "new Either");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<Void, std::string>
  {
    co_await awaiter;
    co_return OK;
  };

  auto originalEither = coro();

  EXPECT_FALSE(originalEither.done());

  auto movedEither = std::move(originalEither);

  EXPECT_FALSE(originalEither.done());
  EXPECT_FALSE(movedEither.done());

  awaiter.resume();

  EXPECT_TRUE(movedEither.done());
  EXPECT_FALSE(movedEither.error());

  EXPECT_FALSE(originalEither.done());
}

TEST(EitherMovePromiseReference, UNIT_054_AsyncMoveWhileSuspended)
{
  RecordProperty("id", "0.02-UNIT-054");
  RecordProperty(
      "desc",
      "Move Either during async operation - promise correctly references new "
      "Either");

  auto coro = []() -> Either<int, std::string>
  {
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30)};
    co_return 555;
  };

  auto originalEither = coro();

  EXPECT_FALSE(originalEither.done());

  // Move immediately (while async operation in flight)
  auto movedEither = std::move(originalEither);

  EXPECT_FALSE(originalEither.done());
  EXPECT_FALSE(movedEither.done());

  // Wait for async completion
  ASSERT_TRUE(pollUntilDone(
      [&] { return movedEither.done(); }, std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_TRUE(movedEither.done());
  EXPECT_TRUE(movedEither.data());
  EXPECT_EQ(*movedEither.data(), 555);

  EXPECT_FALSE(originalEither.done());
}

// NOLINTEND(readability-magic-numbers, readability-identifier-naming)
