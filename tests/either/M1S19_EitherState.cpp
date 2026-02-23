// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include <optional>
#include <string>

#include "TestHelpers.hpp"
#include "utils/SafeAdapterFixtures.hpp"

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
using namespace ropic;

// =============================================================================
// Local Fixtures — Hold ResumeSource without calling requestResume()
// =============================================================================

namespace
{

template <typename T>
struct HoldResumeAwaiter
{
  std::optional<ropic::ResumeSource>* held;
  T value;

  auto await_ready() -> bool { return false; }

  void await_suspend(ropic::ResumeSource rs) { held->emplace(std::move(rs)); }

  auto await_resume() -> T { return std::move(value); }
};

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

/// Resume source that corrupts the state by writing RESUMED directly,
/// bypassing the READY phase. Used to trigger B7 in EitherImpl::state().
struct CorruptResumeSource
{
  std::atomic<ropic::detail::ResumePhase>* state;

  CorruptResumeSource(std::atomic<ropic::detail::ResumePhase>& s)
      : state(&s)
  {
  }

  void requestResume() const noexcept
  {
    state->store(
        ropic::detail::ResumePhase::RESUMED, std::memory_order_seq_cst);
    state->notify_one();
  }
};

/// Void awaiter using CorruptResumeSource, inline resume.
struct CorruptResumeAwaiter
{
  auto await_ready() -> bool { return false; }

  void await_suspend(CorruptResumeSource rs) { rs.requestResume(); }

  void await_resume() {}
};

} // namespace

// =============================================================================
// Part A: DONE State (Branches B2, B3)
// =============================================================================

TEST(M1S19EitherState, U01DoneAfterCoReturnValue)
{
  RecordProperty("id", "M1-S19-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() returns DONE after co_return value (B2: result.has_value())");

  auto result = returnData(42);
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S19EitherState, U02DoneAfterCoReturnError)
{
  RecordProperty("id", "M1-S19-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() returns DONE after co_return error (B3: error != nullptr)");

  auto result = returnError("err");
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(*result.error(), "err");
}

TEST(M1S19EitherState, U03DoneAfterCoReturnVoidOK)
{
  RecordProperty("id", "M1-S19-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() returns DONE after co_return OK on Either<Void, string>");

  auto result = returnOK();
  ASSERT_EQ(result.state(), CoroState::DONE);
  EXPECT_FALSE(result.error());
}

TEST(M1S19EitherState, U04DoneAfterChainedCoAwait)
{
  RecordProperty("id", "M1-S19-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "state() returns DONE after chained co_await all succeed");

  auto coro = [](int start) -> Either<int, std::string>
  {
    int a = co_await returnData(start);
    int b = co_await returnData(a + 10);
    int c = co_await returnData(b + 100);
    co_return c;
  };

  auto result = coro(1);
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 111);
}

// =============================================================================
// Part B: UNDEFINED State (Branches B1, B4)
// =============================================================================

TEST(M1S19EitherState, U05UndefinedAfterMoveConstruct)
{
  RecordProperty("id", "M1-S19-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() returns UNDEFINED on moved-from Either "
      "(B1: _handle == nullptr)");

  auto src = returnData(42);
  ASSERT_EQ(src.state(), CoroState::DONE);

  auto dst{std::move(src)};
  EXPECT_EQ(src.state(), CoroState::UNDEFINED);

  ASSERT_EQ(dst.state(), CoroState::DONE);
  ASSERT_TRUE(dst.value());
  EXPECT_EQ(*dst.value(), 42);
}

TEST(M1S19EitherState, U06UndefinedAfterMoveAssign)
{
  RecordProperty("id", "M1-S19-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() returns UNDEFINED on moved-from Either after "
      "move assignment (B1: _handle == nullptr)");

  auto src = returnData(42);
  auto dst = returnData(0);
  ASSERT_EQ(src.state(), CoroState::DONE);
  ASSERT_EQ(dst.state(), CoroState::DONE);

  dst = std::move(src);
  EXPECT_EQ(src.state(), CoroState::UNDEFINED);

  ASSERT_EQ(dst.state(), CoroState::DONE);
  ASSERT_TRUE(dst.value());
  EXPECT_EQ(*dst.value(), 42);
}

TEST(M1S19EitherState, U07UndefinedSuspendedViaStandardAwaiter)
{
  RecordProperty("id", "M1-S19-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() returns UNDEFINED when suspended via standard awaiter "
      "(B4: !resumeTarget, no SafeAwaitableAdapter)");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 42;
  };

  auto result = coro();
  EXPECT_EQ(result.state(), CoroState::UNDEFINED);

  awaiter.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
}

TEST(M1S19EitherState, U08UndefinedStandardAwaiterThenDone)
{
  RecordProperty("id", "M1-S19-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() transitions UNDEFINED to DONE after standard "
      "awaiter resumes (B4 then B2)");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 99;
  };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::UNDEFINED);

  awaiter.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 99);
}

// =============================================================================
// Part C: PENDING State (Branch B5)
// =============================================================================

TEST(M1S19EitherState, U09PendingBeforeRequestResume)
{
  RecordProperty("id", "M1-S19-U09");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() returns PENDING when safe awaitable suspends but "
      "requestResume() not yet called (B5: SUSPENDED)");

  std::optional<ropic::ResumeSource> held;

  auto coro = [&held]() -> Either<int, std::string>
  {
    co_await HoldResumeVoidAwaiter{.held = &held};
    co_return 42;
  };

  auto result = coro();
  ASSERT_TRUE(held.has_value());
  EXPECT_EQ(result.state(), CoroState::PENDING);

  // Clean up: complete the coroutine to avoid dangling frame
  held->requestResume();
  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
}

// =============================================================================
// Part D: READY State (Branch B6)
// =============================================================================

TEST(M1S19EitherState, U10ReadyAfterInlineResume)
{
  RecordProperty("id", "M1-S19-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() returns READY after inline requestResume() "
      "(B6: READY)");

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

TEST(M1S19EitherState, U11ReadyAfterExternalRequestResume)
{
  RecordProperty("id", "M1-S19-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() transitions PENDING to READY after external "
      "requestResume() (B5 then B6)");

  std::optional<ropic::ResumeSource> held;

  auto coro = [&held]() -> Either<int, std::string>
  { co_return co_await HoldResumeAwaiter<int>{.held = &held, .value = 42}; };

  auto result = coro();
  ASSERT_TRUE(held.has_value());
  ASSERT_EQ(result.state(), CoroState::PENDING);

  held->requestResume();
  EXPECT_EQ(result.state(), CoroState::READY);

  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

// =============================================================================
// Part E: Full State Transitions
// =============================================================================

TEST(M1S19EitherState, U12RepeatedStandardAwaitersInLoop)
{
  RecordProperty("id", "M1-S19-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "10 sequential co_await with standard awaiter; "
      "each iteration yields UNDEFINED until final DONE");

  ManualResumeAwaiter manual;

  auto coro = [&manual]() -> Either<int, std::string>
  {
    int sum = 0;
    for (int i = 0; i < 10; ++i)
    {
      co_await manual;
      sum += i;
    }
    co_return sum;
  };

  auto result = coro();

  for (int i = 0; i < 10; ++i)
  {
    SCOPED_TRACE("iteration " + std::to_string(i));
    ASSERT_EQ(result.state(), CoroState::UNDEFINED);
    manual.resume();
  }

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 45);
}

TEST(M1S19EitherState, U13RepeatedHoldAwaitersInLoop)
{
  RecordProperty("id", "M1-S19-U13");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "10 sequential co_await with held safe awaiter; "
      "each iteration yields PENDING until final DONE");

  std::optional<ropic::ResumeSource> held;

  auto coro = [&held]() -> Either<int, std::string>
  {
    int sum = 0;
    for (int i = 0; i < 10; ++i)
    {
      co_await HoldResumeVoidAwaiter{.held = &held};
      sum += i;
    }
    co_return sum;
  };

  auto result = coro();

  for (int i = 0; i < 10; ++i)
  {
    SCOPED_TRACE("iteration " + std::to_string(i));
    ASSERT_TRUE(held.has_value());
    ASSERT_EQ(result.state(), CoroState::PENDING);
    held->requestResume();
    ASSERT_EQ(result.state(), CoroState::READY);
    result.resume();
  }

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 45);
}

TEST(M1S19EitherState, U14RepeatedInlineAwaitablesInLoop)
{
  RecordProperty("id", "M1-S19-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "10 sequential co_await with INLINE resume; "
      "each resume() yields READY until final DONE");

  auto coro = []() -> Either<int, std::string>
  {
    int sum = 0;
    for (int i = 0; i < 10; ++i)
    {
      sum += co_await ValueResumeAwaiter<int>{
          .value = i, .mode = ResumeMode::INLINE};
    }
    co_return sum;
  };

  auto result = coro();

  for (int i = 0; i < 9; ++i)
  {
    SCOPED_TRACE("iteration " + std::to_string(i));
    ASSERT_EQ(result.state(), CoroState::READY);
    result.resume();
  }

  // Last resume completes the coroutine
  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 45);
}

TEST(M1S19EitherState, U15PendingToReadyToDoneWithError)
{
  RecordProperty("id", "M1-S19-U15");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Full lifecycle with error: PENDING -> READY -> DONE "
      "(B5 then B6 then B3)");

  std::optional<ropic::ResumeSource> held;

  auto coro = [&held]() -> Either<int, std::string>
  {
    co_await HoldResumeVoidAwaiter{.held = &held};
    co_return std::string("held error");
  };

  auto result = coro();
  ASSERT_TRUE(held.has_value());
  ASSERT_EQ(result.state(), CoroState::PENDING);

  held->requestResume();
  ASSERT_EQ(result.state(), CoroState::READY);

  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(*result.error(), "held error");
}

TEST(M1S19EitherState, U16StandardThenSafeAwaitable)
{
  RecordProperty("id", "M1-S19-U16");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Standard awaiter then safe awaitable: "
      "UNDEFINED -> READY -> DONE (B4 then B6 then B2)");

  ManualResumeAwaiter manual;

  auto coro = [&manual]() -> Either<int, std::string>
  {
    co_await manual;
    co_await VoidResumeAwaiter{.mode = ResumeMode::INLINE};
    co_return 42;
  };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::UNDEFINED);

  manual.resume();
  ASSERT_EQ(result.state(), CoroState::READY);

  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S19EitherState, U17SafeThenStandardAwaitable)
{
  RecordProperty("id", "M1-S19-U17");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Safe awaitable then standard awaiter: "
      "READY -> UNDEFINED -> DONE (B6 then B4 then B2)");

  ManualResumeAwaiter manual;

  auto coro = [&manual]() -> Either<int, std::string>
  {
    co_await VoidResumeAwaiter{.mode = ResumeMode::INLINE};
    co_await manual;
    co_return 42;
  };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::READY);

  result.resume();
  ASSERT_EQ(result.state(), CoroState::UNDEFINED);

  manual.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

// =============================================================================
// Part F: noexcept Verification
// =============================================================================

TEST(M1S19EitherState, U18StateIsNoexcept)
{
  RecordProperty("id", "M1-S19-U18");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "state() is noexcept on const Either<int, std::string>&");

  EXPECT_TRUE(
      noexcept(std::declval<const Either<int, std::string>&>().state()));
}

// =============================================================================
// Part G: Death Test — Unreachable RESUMED Phase (Branch B7)
// =============================================================================

TEST(M1S19EitherState, U19ResumedPhaseTriggersAssert)
{
  RecordProperty("id", "M1-S19-U19");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "state() triggers assert when resumeTarget.state() returns "
      "RESUMED (B7: unreachable through normal API)");

  auto coro = []() -> Either<int, std::string>
  {
    co_await CorruptResumeAwaiter{};
    co_return 1;
  };

  auto result = coro();

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH((void)result.state(), "");
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
