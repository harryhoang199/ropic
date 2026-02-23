// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <atomic>
#include <gtest/gtest.h>

#include "ropic/detail/shared/resume_phase.hpp"

#include "ropic/detail/either/resume_source.hpp"

using ropic::ResumeSource;
using ropic::detail::ResumePhase;

// NOLINTBEGIN(readability-magic-numbers)

// =============================================================================
// Part A: Constructor/Assignment + requestResume() (U01-U06)
// =============================================================================

TEST(M1S15_ResumeSource, U01_ParameterizedCtorBindsState)
{
  RecordProperty("id", "M1-S15-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Parameterized constructor binds _state; requestResume transitions "
      "SUSPENDED to READY");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource rs(state);

  rs.requestResume();

  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

TEST(M1S15_ResumeSource, U02_CopyCtorSharesStatePointer)
{
  RecordProperty("id", "M1-S15-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Copy constructor shares state pointer; copy's requestResume affects "
      "shared state");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource original(state);
  ResumeSource copy(original);

  copy.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);

  // Reset and verify original still valid
  state.store(ResumePhase::SUSPENDED, std::memory_order_seq_cst);
  original.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

TEST(M1S15_ResumeSource, U03_CopyAssignRebindsState)
{
  RecordProperty("id", "M1-S15-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Copy assignment rebinds target to source's state; old binding "
      "abandoned");

  std::atomic<ResumePhase> s1{ResumePhase::SUSPENDED};
  std::atomic<ResumePhase> s2{ResumePhase::SUSPENDED};
  ResumeSource a(s1);
  ResumeSource b(s2);

  b = a;
  b.requestResume();

  EXPECT_EQ(s1.load(std::memory_order_seq_cst), ResumePhase::READY);
  EXPECT_EQ(s2.load(std::memory_order_seq_cst), ResumePhase::SUSPENDED);
}

TEST(M1S15_ResumeSource, U04_MoveCtorTransfersOwnership)
{
  RecordProperty("id", "M1-S15-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Move constructor transfers ownership; source's requestResume asserts");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource original(state);
  ResumeSource moved(std::move(original));

  moved.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(original.requestResume(), "");
#endif
}

TEST(M1S15_ResumeSource, U05_MoveAssignTransfersOwnership)
{
  RecordProperty("id", "M1-S15-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Move assignment transfers ownership; source's requestResume asserts");

  std::atomic<ResumePhase> s1{ResumePhase::SUSPENDED};
  std::atomic<ResumePhase> s2{ResumePhase::SUSPENDED};
  ResumeSource a(s1);
  ResumeSource b(s2);

  b = std::move(a);
  b.requestResume();

  EXPECT_EQ(s1.load(std::memory_order_seq_cst), ResumePhase::READY);
  EXPECT_EQ(s2.load(std::memory_order_seq_cst), ResumePhase::SUSPENDED);

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(a.requestResume(), "");
#endif
}

TEST(M1S15_ResumeSource, U06_SelfMoveAssignPreservesValidity)
{
  RecordProperty("id", "M1-S15-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Self-move-assignment preserves object validity via self-assignment "
      "guard");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource rs(state);

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wself-move"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wself-move"
#endif
  rs = std::move(rs);
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

  rs.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

// =============================================================================
// Part B: requestResume() Behavioral Tests (U07-U10)
// =============================================================================

TEST(M1S15_ResumeSource, U07_RequestResumeNoOpWhenReady)
{
  RecordProperty("id", "M1-S15-U07");
  RecordProperty("ver", "0.05");
  RecordProperty("desc", "requestResume is no-op when state is already READY");

  std::atomic<ResumePhase> state{ResumePhase::READY};
  ResumeSource rs(state);

  rs.requestResume();

  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

TEST(M1S15_ResumeSource, U08_RequestResumeNoOpWhenResumed)
{
  RecordProperty("id", "M1-S15-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "requestResume is no-op when state is already RESUMED");

  std::atomic<ResumePhase> state{ResumePhase::RESUMED};
  ResumeSource rs(state);

  rs.requestResume();

  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::RESUMED);
}

TEST(M1S15_ResumeSource, U09_ConsecutiveCallsIdempotent)
{
  RecordProperty("id", "M1-S15-U09");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Consecutive requestResume calls are idempotent after first transition");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource rs(state);

  rs.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);

  rs.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);

  rs.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

TEST(M1S15_ResumeSource, U10_MultipleCopiesFirstWins)
{
  RecordProperty("id", "M1-S15-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Multiple copies share atomic; first requestResume wins, rest are "
      "no-ops");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource src(state);
  ResumeSource c1(src);
  ResumeSource c2(src);
  ResumeSource c3(src);

  c1.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);

  c2.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);

  c3.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

// =============================================================================
// Part C: Compound Operations (U11-U13)
// =============================================================================

TEST(M1S15_ResumeSource, U11_MoveChainOnlyFinalWorks)
{
  RecordProperty("id", "M1-S15-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "Chain of three moves leaves only the final object functional");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource a(state);
  ResumeSource b(std::move(a));
  ResumeSource c(std::move(b));

  c.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(a.requestResume(), "");
  EXPECT_DEBUG_DEATH(b.requestResume(), "");
#endif
}

TEST(M1S15_ResumeSource, U12_CopySurvivesSourceMove)
{
  RecordProperty("id", "M1-S15-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "A copy remains valid after the original is moved from");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource original(state);
  ResumeSource copy(original);
  ResumeSource moved(std::move(original));

  copy.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(original.requestResume(), "");
#endif
}

TEST(M1S15_ResumeSource, U13_MoveAssignOverwritesBinding)
{
  RecordProperty("id", "M1-S15-U13");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Move assignment replaces existing binding, orphaning the old atomic");

  std::atomic<ResumePhase> s1{ResumePhase::SUSPENDED};
  std::atomic<ResumePhase> s2{ResumePhase::SUSPENDED};
  ResumeSource a(s1);
  ResumeSource b(s2);

  a = std::move(b);
  a.requestResume();

  EXPECT_EQ(s2.load(std::memory_order_seq_cst), ResumePhase::READY);
  EXPECT_EQ(s1.load(std::memory_order_seq_cst), ResumePhase::SUSPENDED);

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(b.requestResume(), "");
#endif
}

// =============================================================================
// Part D: Edge Cases (U14-U19)
// =============================================================================

TEST(M1S15_ResumeSource, U14_CopyFromMovedFromProducesNull)
{
  RecordProperty("id", "M1-S15-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Copy-constructing from a moved-from object produces a null copy");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource a(state);
  ResumeSource b(std::move(a));

  [[maybe_unused]]
  ResumeSource nullCopy(a);

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(nullCopy.requestResume(), "");
#endif

  b.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

TEST(M1S15_ResumeSource, U15_CopyAssignFromMovedFromProducesNull)
{
  RecordProperty("id", "M1-S15-U15");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Copy-assigning from a moved-from object copies the null pointer");

  std::atomic<ResumePhase> s1{ResumePhase::SUSPENDED};
  std::atomic<ResumePhase> s2{ResumePhase::SUSPENDED};
  ResumeSource a(s1);
  ResumeSource b(s2);
  ResumeSource c(std::move(a)); // a is now null

  b = a; // copy-assign from moved-from

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(b.requestResume(), "");
#endif
}

TEST(M1S15_ResumeSource, U16_SelfCopyAssignPreservesValidity)
{
  RecordProperty("id", "M1-S15-U16");
  RecordProperty("ver", "0.05");
  RecordProperty("desc", "Self-copy-assignment does not invalidate the object");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource a(state);

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wself-assign-overloaded"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wself-assign"
#endif
  a = a;
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

  a.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

TEST(M1S15_ResumeSource, U17_MoveFromMovedFromProducesNull)
{
  RecordProperty("id", "M1-S15-U17");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Move-constructing from a moved-from object produces a null object");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource a(state);
  ResumeSource b(std::move(a)); // a is now null

  ResumeSource nullMoved(std::move(a)); // move from moved-from

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(nullMoved.requestResume(), "");
#endif

  b.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

TEST(M1S15_ResumeSource, U18_MoveAssignFromMovedFromProducesNull)
{
  RecordProperty("id", "M1-S15-U18");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "Move-assigning from a moved-from object makes the target null");

  std::atomic<ResumePhase> s1{ResumePhase::SUSPENDED};
  std::atomic<ResumePhase> s2{ResumePhase::SUSPENDED};
  ResumeSource a(s1);
  ResumeSource b(s2);
  ResumeSource c(std::move(a)); // a is now null

  b = std::move(a); // move-assign from moved-from

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(b.requestResume(), "");
#endif

  c.requestResume();
  EXPECT_EQ(s1.load(std::memory_order_seq_cst), ResumePhase::READY);
  EXPECT_EQ(s2.load(std::memory_order_seq_cst), ResumePhase::SUSPENDED);
}

TEST(M1S15_ResumeSource, U19_SelfMoveAssignOnMovedFromRemainsNull)
{
  RecordProperty("id", "M1-S15-U19");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Self-move-assignment on a moved-from object does not crash; object "
      "stays null");

  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
  ResumeSource a(state);
  ResumeSource b(std::move(a)); // a is now null

#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wself-move"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wself-move"
#endif
  a = std::move(a); // self-move on moved-from
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif

#ifndef NDEBUG
  GTEST_FLAG_SET(death_test_style, "threadsafe");
  EXPECT_DEBUG_DEATH(a.requestResume(), "");
#endif

  b.requestResume();
  EXPECT_EQ(state.load(std::memory_order_seq_cst), ResumePhase::READY);
}

// NOLINTEND(readability-magic-numbers)
