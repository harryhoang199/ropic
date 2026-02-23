// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <atomic>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <thread>

#ifdef NDEBUG
#  include "ropic.hpp"
#else
#  include "TestHelpers.hpp"
#endif
#include "utils/SafeAdapterFixtures.hpp"
#include "utils/StateControlRS.hpp"

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
using namespace ropic;

// =============================================================================
// Local Fixtures
// =============================================================================

namespace
{

/// Captures the raw ResumeHead state pointer without calling requestResume(),
/// leaving state at SUSPENDED and giving the test full timing control.
struct HoldStateAwaiter
{
  std::atomic<ropic::detail::ResumePhase>*& statePtr;

  auto await_ready() -> bool { return false; }

  void await_suspend(StateControlRS rs) const { statePtr = &rs.state; }

  void await_resume() {}
};

/// Stores ResumeSource externally without calling requestResume(), leaving
/// state at SUSPENDED and giving the test control over signalling.
struct HoldResumeVoidAwaiter
{
  std::optional<ropic::ResumeSource>* held;

  auto await_ready() -> bool { return false; }

  void await_suspend(ropic::ResumeSource rs) const
  {
    held->emplace(std::move(rs));
  }

  void await_resume() {}
};

} // namespace

// =============================================================================
// Part A: Normal Resume — State READY When resume() Called
// =============================================================================

TEST(M1S20EitherResume, U01ResumeAfterInlineRequestResume)
{
  RecordProperty("id", "M1-S20-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() completes coroutine when requestResume() was called inline "
      "during await_suspend, so state is already READY on entry");

  auto coro = []() -> Either<int, std::string>
  {
    co_await VoidResumeAwaiter{.mode = ResumeMode::INLINE};
    co_return 42;
  };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::READY);

  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S20EitherResume, U02ResumeAfterExternalRequestResume)
{
  RecordProperty("id", "M1-S20-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() completes coroutine after external requestResume() "
      "transitions state from PENDING to READY");

  std::optional<ropic::ResumeSource> held;

  auto coro = [&held]() -> Either<int, std::string>
  {
    co_await HoldResumeVoidAwaiter{.held = &held};
    co_return 42;
  };

  auto result = coro();
  ASSERT_TRUE(held.has_value());
  ASSERT_EQ(result.state(), CoroState::PENDING);

  held->requestResume();
  ASSERT_EQ(result.state(), CoroState::READY);

  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S20EitherResume, U03ResumeCompletesWithError)
{
  RecordProperty("id", "M1-S20-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() completes coroutine that co_returns an error value; "
      "error() is accessible after resume");

  auto coro = []() -> Either<int, std::string>
  {
    co_await VoidResumeAwaiter{.mode = ResumeMode::INLINE};
    co_return std::string("oops");
  };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::READY);

  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(*result.error(), "oops");
}

// =============================================================================
// Part B: Blocking Wait — resume() Blocks Until State Becomes READY
// =============================================================================

TEST(M1S20EitherResume, U04ResumeAfterReadySignaledFromThread)
{
  RecordProperty("id", "M1-S20-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "background thread signals READY before resume() is called, "
      "so resume() completes immediately without blocking");

  std::atomic<ropic::detail::ResumePhase>* statePtr = nullptr;

  auto coro = [&statePtr]() -> Either<int, std::string>
  {
    co_await HoldStateAwaiter{.statePtr = statePtr};
    co_return 42;
  };

  auto result = coro();
  ASSERT_NE(statePtr, nullptr);
  ASSERT_EQ(result.state(), CoroState::PENDING);

  std::atomic_bool ready{false};

  std::jthread t(
      [statePtr, &ready]
      {
        auto expected = ropic::detail::ResumePhase::SUSPENDED;
        statePtr->compare_exchange_strong(
            expected,
            ropic::detail::ResumePhase::READY,
            std::memory_order_seq_cst);
        statePtr->notify_one();
        ready.store(true, std::memory_order_release);
        ready.notify_one();
      });

  ready.wait(false, std::memory_order_acquire);
  result.resume(); // READY already signaled, completes immediately

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S20EitherResume, U05ResumeBlocksUntilReadySignaledFromThread)
{
  RecordProperty("id", "M1-S20-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() is called before the background thread signals READY, "
      "so resume() blocks until the thread transitions state to READY");

  std::atomic<ropic::detail::ResumePhase>* statePtr = nullptr;

  auto coro = [&statePtr]() -> Either<int, std::string>
  {
    co_await HoldStateAwaiter{.statePtr = statePtr};
    co_return 42;
  };

  auto result = coro();
  ASSERT_NE(statePtr, nullptr);
  ASSERT_EQ(result.state(), CoroState::PENDING);

  std::atomic_bool resumed{false};

  std::jthread t(
      [statePtr, &resumed]
      {
        resumed.wait(false, std::memory_order_acquire);
        auto expected = ropic::detail::ResumePhase::SUSPENDED;
        statePtr->compare_exchange_strong(
            expected,
            ropic::detail::ResumePhase::READY,
            std::memory_order_seq_cst);
        statePtr->notify_one();
      });

  resumed.store(true, std::memory_order_release);
  resumed.notify_one();
  result.resume(); // blocks until thread signals READY

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

// =============================================================================
// Part C: Early Return — State Already RESUMED, Coroutine Not Resumed
// =============================================================================

TEST(M1S20EitherResume, U06ResumeReturnsEarlyWhenStateCorruptedToResumed)
{
  RecordProperty("id", "M1-S20-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() returns early without resuming coroutine when state is "
      "already RESUMED; CAS(READY→RESUMED) fails on first attempt");

  std::atomic<ropic::detail::ResumePhase>* statePtr = nullptr;

  auto coro = [&statePtr]() -> Either<int, std::string>
  {
    co_await HoldStateAwaiter{.statePtr = statePtr};
    co_return 42;
  };

  auto result = coro();
  ASSERT_NE(statePtr, nullptr);

  // Corrupt state to RESUMED directly — bypasses READY phase
  statePtr->store(
      ropic::detail::ResumePhase::RESUMED, std::memory_order_seq_cst);

  result.resume(); // CAS(READY→RESUMED) fails because state is already RESUMED
                   // → returns early without resuming the coroutine

  // Coroutine was NOT resumed: co_return 42 never executed
  EXPECT_FALSE(result.value());
  EXPECT_FALSE(result.error());

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH((void)result.state(), "");
}

TEST(M1S20EitherResume, U07ResumeAfterResumedSignaledFromThread)
{
  RecordProperty("id", "M1-S20-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "background thread writes RESUMED (not READY) before resume() is "
      "called, so resume() returns early without resuming coroutine");

  std::atomic<ropic::detail::ResumePhase>* statePtr = nullptr;

  auto coro = [&statePtr]() -> Either<int, std::string>
  {
    co_await HoldStateAwaiter{.statePtr = statePtr};
    co_return 42;
  };

  auto result = coro();
  ASSERT_NE(statePtr, nullptr);

  std::atomic_bool ready{false};

  std::jthread t(
      [statePtr, &ready]
      {
        statePtr->store(
            ropic::detail::ResumePhase::RESUMED, std::memory_order_seq_cst);
        statePtr->notify_one();
        ready.store(true, std::memory_order_release);
        ready.notify_one();
      });

  ready.wait(false, std::memory_order_acquire);
  result.resume(); // RESUMED already written, CAS(READY→RESUMED) fails
                   // → returns early without resuming coroutine

  // Coroutine was NOT resumed: co_return 42 never executed
  EXPECT_FALSE(result.value());
  EXPECT_FALSE(result.error());

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH((void)result.state(), "");
}

TEST(M1S20EitherResume, U08ResumeBlocksUntilResumedSignaledFromThread)
{
  RecordProperty("id", "M1-S20-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() blocks while state is SUSPENDED then returns early without "
      "resuming coroutine when background thread writes RESUMED instead of "
      "READY");

  std::atomic<ropic::detail::ResumePhase>* statePtr = nullptr;

  auto coro = [&statePtr]() -> Either<int, std::string>
  {
    co_await HoldStateAwaiter{.statePtr = statePtr};
    co_return 42;
  };

  auto result = coro();
  ASSERT_NE(statePtr, nullptr);

  std::atomic_bool resumed{false};

  std::jthread t(
      [statePtr, &resumed]
      {
        resumed.wait(false, std::memory_order_acquire);
        statePtr->store(
            ropic::detail::ResumePhase::RESUMED, std::memory_order_seq_cst);
        statePtr->notify_one();
      });

  resumed.store(true, std::memory_order_release);
  resumed.notify_one();
  result.resume(); // blocks on SUSPENDED, wakes when state=RESUMED,
                   // CAS(READY→RESUMED) fails → returns early

  // Coroutine was NOT resumed: co_return 42 never executed
  EXPECT_FALSE(result.value());
  EXPECT_FALSE(result.error());

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH((void)result.state(), "");
}

// =============================================================================
// Part D: noexcept Verification
// =============================================================================

TEST(M1S20EitherResume, U09ResumeIsNoexcept)
{
  RecordProperty("id", "M1-S20-U09");
  RecordProperty("ver", "0.05");
  RecordProperty("desc", "resume() is noexcept on Either<int, std::string>&");

  EXPECT_TRUE(noexcept(std::declval<Either<int, std::string>&>().resume()));
}

// =============================================================================
// Part E: Assertion Paths — Death Tests
// =============================================================================

TEST(M1S20EitherResume, U10ResumeOnMovedFromTriggersAssert)
{
  RecordProperty("id", "M1-S20-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() on moved-from Either triggers assert "
      "because the coroutine handle is null after move");

#ifdef NDEBUG
  GTEST_SKIP() << "Assertion checks are disabled in release builds";
#else
  auto src = returnData(42);
  [[maybe_unused]]
  auto dst{std::move(src)};

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(src.resume(), "");
#endif
}

TEST(M1S20EitherResume, U11ResumeOnDoneTriggersAssert)
{
  RecordProperty("id", "M1-S20-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() on already-completed Either triggers assert "
      "because no suspension occurred and resumeTarget is null");

#ifdef NDEBUG
  GTEST_SKIP() << "Assertion checks are disabled in release builds";
#else
  auto result = returnData(42);
  ASSERT_EQ(result.state(), CoroState::DONE);

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(result.resume(), "");
#endif
}

TEST(M1S20EitherResume, U12ResumeOnStandardAwaiterTriggersAssert)
{
  RecordProperty("id", "M1-S20-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "resume() when suspended via standard awaiter triggers assert "
      "because ManualResumeAwaiter bypasses SafeAwaitableAdapter "
      "and sets no resumeTarget");

#ifdef NDEBUG
  GTEST_SKIP() << "Assertion checks are disabled in release builds";
#else
  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 42;
  };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::UNDEFINED);

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(result.resume(), "");

  // Cleanup: advance the coroutine to avoid destroying a suspended frame
  // that holds a reference to the stack-allocated awaiter
  awaiter.resume();
#endif
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
