// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <atomic>
#include <gtest/gtest.h>
#include <string>
#include <thread>

#include "ropic.hpp"
#include "utils/StateControlRS.hpp"
#include "utils/TaggedError.hpp"

#include "ropic/detail/either/resume_source.hpp"

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
using namespace ropic;

#ifndef ROPIC_TESTING_MODE
#  error "ROPIC_TESTING_MODE must be defined for this test file"
#endif

// =============================================================================
// Local Fixtures
// =============================================================================

namespace
{
// =============================================================================
// Gate Alias
// =============================================================================

/// @brief Alias for the PropagatingAwaiter specialization whose
/// s_awaitSuspendGate controls suspension propagation timing.
/// Each unique TaggedError<ID> produces a distinct gate.
template <FixedString ID>
using Gate = ropic::detail::
    PropagatingAwaiter<Void, TaggedError<ID>, Void, TaggedError<ID>, false>;

/// @brief Safe awaiter for normal tests. Spawns a background thread that:
/// 1. Spins until s_awaitSuspendGate == 0 (gate closed)
/// 2. Calls rs.requestResume() → SUSPENDED→READY
/// 3. Decrements gate by 1 → unblocks the waiting PropagatingAwaiter
template <FixedString ID>
struct GatedResumeAwaiter
{
  [[nodiscard]]
  auto await_ready() const -> bool
  {
    return false;
  }

  void await_suspend(ropic::ResumeSource rs) const
  {
    std::thread(
        [rs]()
        {
          while (Gate<ID>::s_awaitSuspendGate.load() != 0)
          {
          }
          rs.requestResume();

          Gate<ID>::s_awaitSuspendGate.fetch_add(-1, std::memory_order_seq_cst);
        })
        .detach();
  }

  void await_resume() const {}
};

/// @brief Safe awaiter for death tests. Spawns a background thread that:
/// 1. Spins until s_awaitSuspendGate == 0 (gate closed)
/// 2. Writes RESUMED directly to state (corrupting the state machine)
/// 3. Decrements gate by 1 → unblocks the waiting PropagatingAwaiter
template <FixedString ID>
struct GatedCorruptAwaiter
{
  auto await_ready() -> bool { return false; }

  void await_suspend(StateControlRS rs)
  {
    std::thread(
        [rs]()
        {
          while (Gate<ID>::s_awaitSuspendGate.load() != 0)
          {
          }

          rs.state.store(
              ropic::detail::ResumePhase::RESUMED, std::memory_order_seq_cst);
          rs.state.notify_one();

          Gate<ID>::s_awaitSuspendGate.fetch_add(-1, std::memory_order_seq_cst);
        })
        .detach();
  }

  void await_resume() {}
};

/// @brief Awaiter that calls requestResume() inline (no background thread).
/// Used with negative gate values where the gate is already open and
/// no synchronization with a background thread is needed.
template <FixedString ID>
struct InlineResumeAwaiter
{
  [[nodiscard]]
  auto await_ready() const -> bool
  {
    return false;
  }

  void await_suspend(ropic::ResumeSource rs) const { rs.requestResume(); }

  void await_resume() const {}
};

// =============================================================================
// 4-Layer Nested Coroutine Chain
// =============================================================================

template <FixedString ID, typename Awaiter>
auto layer3(Awaiter& awaiter) -> Either<Void, TaggedError<ID>>
{
  co_await awaiter;
  co_return OK;
}

template <FixedString ID, typename Awaiter>
auto layer2(Awaiter& awaiter) -> Either<Void, TaggedError<ID>>
{
  co_await layer3<ID>(awaiter);
  co_return OK;
}

template <FixedString ID, typename Awaiter>
auto layer1(Awaiter& awaiter) -> Either<Void, TaggedError<ID>>
{
  co_await layer2<ID>(awaiter);
  co_return OK;
}

template <FixedString ID, typename Awaiter>
auto outermost(Awaiter& awaiter) -> Either<Void, TaggedError<ID>>
{
  co_await layer1<ID>(awaiter);
  co_return OK;
}

} // namespace

// =============================================================================
// Part A: Normal Tests — All gates pre-opened (no blocking)
// =============================================================================

TEST(M1S21NestedSuspendPropagation, U01AllGatesPreOpenedAllLayersFindSuspended)
{
  RecordProperty("id", "M1-S21-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "gate=-2 pre-opens all gates; inline requestResume() runs before "
      "any PropagatingAwaiter::await_suspend; all 3 layers find READY "
      "via tryClaimHandle and perform symmetric transfer immediately");

#ifndef ROPIC_TESTING_MODE
  GTEST_SKIP() << "Requires ROPIC_TESTING_MODE";
#endif

  constexpr auto ID = FixedString{"M1-S21-U01"};
  Gate<ID>::s_awaitSuspendGate.store(-2, std::memory_order_seq_cst);

  InlineResumeAwaiter<ID> awaiter;
  auto result = outermost<ID>(awaiter);

  ASSERT_EQ(result.state(), CoroState::DONE);
  EXPECT_FALSE(result.error());
}

// =============================================================================
// Part B: Normal Tests — TH1: requestResume BEFORE tryClaimHandle
// =============================================================================

TEST(M1S21NestedSuspendPropagation, U02RequestResumeBeforeLayer2TryClaimHandle)
{
  RecordProperty("id", "M1-S21-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "requestResume() called before layer2's tryClaimHandle(); "
      "gate=0 blocks layer2's PA, background thread signals READY "
      "first, CAS(READY->RESUMED) succeeds at layer2");

#ifndef ROPIC_TESTING_MODE
  GTEST_SKIP() << "Requires ROPIC_TESTING_MODE";
#endif

  constexpr auto ID = FixedString{"M1-S21-U02"};
  Gate<ID>::s_awaitSuspendGate.store(0, std::memory_order_seq_cst);

  GatedResumeAwaiter<ID> awaiter;
  auto result = outermost<ID>(awaiter);

  EXPECT_LT(Gate<ID>::s_awaitSuspendGate, 0);

  ASSERT_EQ(result.state(), CoroState::DONE);
  EXPECT_FALSE(result.error());
}

TEST(M1S21NestedSuspendPropagation, U03RequestResumeBeforeLayer1TryClaimHandle)
{
  RecordProperty("id", "M1-S21-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "requestResume() called before layer1's tryClaimHandle(); "
      "gate=1 lets layer2 pass (SUSPENDED), blocks layer1's PA, "
      "background thread signals READY, CAS succeeds at layer1");

#ifndef ROPIC_TESTING_MODE
  GTEST_SKIP() << "Requires ROPIC_TESTING_MODE";
#endif

  constexpr auto ID = FixedString{"M1-S21-U03"};
  Gate<ID>::s_awaitSuspendGate.store(1, std::memory_order_seq_cst);

  GatedResumeAwaiter<ID> awaiter;
  auto result = outermost<ID>(awaiter);

  EXPECT_LT(Gate<ID>::s_awaitSuspendGate, 0);

  ASSERT_EQ(result.state(), CoroState::DONE);
  EXPECT_FALSE(result.error());
}

TEST(
    M1S21NestedSuspendPropagation,
    U04RequestResumeBeforeOutermostTryClaimHandle)
{
  RecordProperty("id", "M1-S21-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "requestResume() called before outermost's tryClaimHandle(); "
      "gate=2 lets layer2 and layer1 pass (SUSPENDED), blocks "
      "outermost's PA, background thread signals READY, "
      "CAS succeeds at outermost");

#ifndef ROPIC_TESTING_MODE
  GTEST_SKIP() << "Requires ROPIC_TESTING_MODE";
#endif

  constexpr auto ID = FixedString{"M1-S21-U04"};
  Gate<ID>::s_awaitSuspendGate.store(2, std::memory_order_seq_cst);

  GatedResumeAwaiter<ID> awaiter;
  auto result = outermost<ID>(awaiter);

  EXPECT_LT(Gate<ID>::s_awaitSuspendGate, 0);

  ASSERT_EQ(result.state(), CoroState::DONE);
  EXPECT_FALSE(result.error());
}

TEST(
    M1S21NestedSuspendPropagation, U05RequestResumeAfterOutermostTryClaimHandle)
{
  RecordProperty("id", "M1-S21-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "requestResume() called after all tryClaimHandle() calls; "
      "gate=3 lets all 3 layers pass and find SUSPENDED, "
      "ResumeTarget propagated to outermost, background thread "
      "signals READY, test calls resume() via waitAndResume");

#ifndef ROPIC_TESTING_MODE
  GTEST_SKIP() << "Requires ROPIC_TESTING_MODE";
#endif

  constexpr auto ID = FixedString{"M1-S21-U05"};
  Gate<ID>::s_awaitSuspendGate.store(3, std::memory_order_seq_cst);

  GatedResumeAwaiter<ID> awaiter;
  auto result = outermost<ID>(awaiter);

  ASSERT_NE(result.state(), CoroState::DONE);
  ASSERT_NE(result.state(), CoroState::UNDEFINED);

  // Background thread eventually signals READY after gate reaches 0
  result.resume();

  EXPECT_LT(Gate<ID>::s_awaitSuspendGate, 0);

  ASSERT_EQ(result.state(), CoroState::DONE);
  EXPECT_FALSE(result.error());
}

// =============================================================================
// Part B: Death Tests — PropagatingAwaiter assert on RESUMED state
// =============================================================================

TEST(
    M1S21NestedSuspendPropagationDeathTest,
    U06PropagatingAwaiterAssertsResumedAtLayer2)
{
  RecordProperty("id", "M1-S21-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "PropagatingAwaiter assert fires at layer2 when state is "
      "corrupted to RESUMED; gate=0 blocks layer2's PA, background "
      "thread writes RESUMED directly, assert(state != RESUMED) fails");

#ifndef ROPIC_TESTING_MODE
  GTEST_SKIP() << "Requires ROPIC_TESTING_MODE";
#else
  constexpr auto ID = FixedString{"M1-S21-U06"};

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(
      {
        Gate<ID>::s_awaitSuspendGate.store(0, std::memory_order_seq_cst);

        GatedCorruptAwaiter<ID> awaiter;
        auto result = outermost<ID>(awaiter);
        (void)result;
      },
      "");
#endif
}

TEST(
    M1S21NestedSuspendPropagationDeathTest,
    U07PropagatingAwaiterAssertsResumedAtLayer1)
{
  RecordProperty("id", "M1-S21-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "PropagatingAwaiter assert fires at layer1 when state is "
      "corrupted to RESUMED; gate=1 lets layer2 pass (SUSPENDED), "
      "blocks layer1, background thread writes RESUMED, "
      "assert(state != RESUMED) fails at layer1");

#ifndef ROPIC_TESTING_MODE
  GTEST_SKIP() << "Requires ROPIC_TESTING_MODE";
#else

  constexpr auto ID = FixedString{"M1-S21-U07"};

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(
      {
        Gate<ID>::s_awaitSuspendGate.store(1, std::memory_order_seq_cst);

        GatedCorruptAwaiter<ID> awaiter;
        auto result = outermost<ID>(awaiter);
        (void)result;
      },
      "");
#endif
}

TEST(
    M1S21NestedSuspendPropagationDeathTest,
    U08PropagatingAwaiterAssertsResumedAtOutermost)
{
  RecordProperty("id", "M1-S21-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "PropagatingAwaiter assert fires at outermost when state is "
      "corrupted to RESUMED; gate=2 lets layer2 and layer1 pass, "
      "blocks outermost, background thread writes RESUMED, "
      "assert(state != RESUMED) fails at outermost");

#ifndef ROPIC_TESTING_MODE
  GTEST_SKIP() << "Requires ROPIC_TESTING_MODE";
#else

  constexpr auto ID = FixedString{"M1-S21-U08"};

  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(
      {
        Gate<ID>::s_awaitSuspendGate.store(2, std::memory_order_seq_cst);

        GatedCorruptAwaiter<ID> awaiter;
        auto result = outermost<ID>(awaiter);
        (void)result;
      },
      "");
#endif
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
