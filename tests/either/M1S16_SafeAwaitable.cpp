// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <coroutine>
#include <gtest/gtest.h>
#include <string>

#include "utils/SafeAwaitableFixtures.hpp"

#include "ropic/safe_awaitable.hpp"

// NOLINTBEGIN(readability-identifier-naming,readability-convert-member-functions-to-static)
namespace
{

// ---- negative fixtures (M1S16-only) ----

struct NoAwaitReady
{
  void await_suspend(ValidRS);
  void await_resume();
};

struct NoAwaitResume
{
  auto await_ready() -> bool;
  void await_suspend(ValidRS);
};

struct NoAwaitSuspend
{
  auto await_ready() -> bool;
  void await_resume();
};

struct ReadyReturnsVoid
{
  void await_ready();
  void await_suspend(ValidRS);
  void await_resume();
};

struct SuspendTakesInt
{
  auto await_ready() -> bool;
  void await_suspend(int);
  void await_resume();
};

struct SuspendNoParam
{
  auto await_ready() -> bool;
  void await_suspend();
  void await_resume();
};

struct OverloadedSuspend
{
  auto await_ready() -> bool;
  void await_suspend(ValidRS);
  void await_suspend(std::coroutine_handle<>);
  void await_resume();
};

struct TemplateSuspend
{
  auto await_ready() -> bool;
  template <typename S>
  void await_suspend(S);
  void await_resume();
};

struct SuspendConstRefNonConstRS
{
  auto await_ready() -> bool;
  void await_suspend(const ValidRS&);
  void await_resume();
};

} // namespace
// NOLINTEND(readability-identifier-naming,readability-convert-member-functions-to-static)

// NOLINTBEGIN(readability-magic-numbers)

// =============================================================================
// Part A: safe_awaitable via safe_awaiter Path (U01-U07)
// =============================================================================

TEST(M1S16SafeAwaitable, U01MinimalSafeAwaiterSatisfies)
{
  RecordProperty("id", "M1-S16-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Minimal awaiter with by-value resume_source param satisfies "
      "safe_awaitable");

  EXPECT_TRUE(ropic::safe_awaitable<MinSafeAwaiter>);
}

TEST(M1S16SafeAwaitable, U02ResumeSourceSatisfies)
{
  RecordProperty("id", "M1-S16-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Awaiter taking ropic::ResumeSource by value satisfies "
      "safe_awaitable");

  EXPECT_TRUE(ropic::safe_awaitable<SuspendTakesResumeSource>);
}

TEST(M1S16SafeAwaitable, U03VariedSignaturesSatisfy)
{
  RecordProperty("id", "M1-S16-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "int await_ready (convertible_to<bool>), bool await_suspend, "
      "int await_resume all satisfy");

  EXPECT_TRUE(ropic::safe_awaitable<VariedSignatures>);
}

TEST(M1S16SafeAwaitable, U04ByRefParamSatisfies)
{
  RecordProperty("id", "M1-S16-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(ValidRS&) by-ref satisfies; resume_source checks "
      "non-const ref");

  EXPECT_TRUE(ropic::safe_awaitable<SuspendTakesRefRS>);
}

TEST(M1S16SafeAwaitable, U05ConstRefParamSatisfies)
{
  RecordProperty("id", "M1-S16-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(const RSConst&) satisfies; RSConst::requestResume "
      "is const");

  EXPECT_TRUE(ropic::safe_awaitable<SuspendTakesConstRef>);
}

TEST(M1S16SafeAwaitable, U06NoexceptMethodsSatisfy)
{
  RecordProperty("id", "M1-S16-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "Awaiter with all noexcept methods satisfies safe_awaitable");

  EXPECT_TRUE(ropic::safe_awaitable<NoexceptAwaiter>);
}

TEST(M1S16SafeAwaitable, U07RvalueRefParamSatisfies)
{
  RecordProperty("id", "M1-S16-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(ValidRS&&) rvalue-ref satisfies; resume_source "
      "callable on rvalue");

  EXPECT_TRUE(ropic::safe_awaitable<SuspendTakesRvalueRefRS>);
}

// =============================================================================
// Part B: safe_awaitable via co_awaitable Path (U08-U10)
// =============================================================================

TEST(M1S16SafeAwaitable, U08MemberCoAwaitableSatisfies)
{
  RecordProperty("id", "M1-S16-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Type with member operator co_await() returning safe_awaiter "
      "satisfies safe_awaitable");

  EXPECT_TRUE(ropic::safe_awaitable<MemberCoAwt>);
}

TEST(M1S16SafeAwaitable, U09NonMemberCoAwaitableSatisfies)
{
  RecordProperty("id", "M1-S16-U09");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Type with free operator co_await(T) returning safe_awaiter "
      "satisfies safe_awaitable");

  EXPECT_TRUE(ropic::safe_awaitable<NonMemberTarget>);
}

TEST(M1S16SafeAwaitable, U10BothOperatorsSatisfy)
{
  RecordProperty("id", "M1-S16-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Type with both member and non-member operator co_await "
      "satisfies safe_awaitable");

  EXPECT_TRUE(ropic::safe_awaitable<BothOperators>);
}

// =============================================================================
// Part C: safe_awaitable — Negative Cases (U11-U20)
// =============================================================================

TEST(M1S16SafeAwaitable, U11StandardAwaiterRejected)
{
  RecordProperty("id", "M1-S16-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Awaiter with await_suspend(coroutine_handle<>) does not "
      "satisfy safe_awaitable");

  EXPECT_FALSE(ropic::safe_awaitable<StandardAwaiter>);
}

TEST(M1S16SafeAwaitable, U12SuspendAlwaysNeverRejected)
{
  RecordProperty("id", "M1-S16-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Standard library awaiters use coroutine_handle<>, not "
      "resume_source");

  EXPECT_FALSE(ropic::safe_awaitable<std::suspend_always>);
  EXPECT_FALSE(ropic::safe_awaitable<std::suspend_never>);
}

TEST(M1S16SafeAwaitable, U13PlainTypesRejected)
{
  RecordProperty("id", "M1-S16-U13");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Plain types with no awaiter interface do not satisfy "
      "safe_awaitable");

  EXPECT_FALSE(ropic::safe_awaitable<int>);
  EXPECT_FALSE(ropic::safe_awaitable<std::string>);
}

TEST(M1S16SafeAwaitable, U14MissingMethodsRejected)
{
  RecordProperty("id", "M1-S16-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Type missing any of the three awaiter methods does not "
      "satisfy safe_awaitable");

  EXPECT_FALSE(ropic::safe_awaitable<NoAwaitReady>);
  EXPECT_FALSE(ropic::safe_awaitable<NoAwaitResume>);
  EXPECT_FALSE(ropic::safe_awaitable<NoAwaitSuspend>);
}

TEST(M1S16SafeAwaitable, U15WrongSuspendParamRejected)
{
  RecordProperty("id", "M1-S16-U15");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(int) and await_suspend() rejected; param must "
      "satisfy resume_source");

  EXPECT_FALSE(ropic::safe_awaitable<SuspendTakesInt>);
  EXPECT_FALSE(ropic::safe_awaitable<SuspendNoParam>);
}

TEST(M1S16SafeAwaitable, U16AwaitReadyVoidRejected)
{
  RecordProperty("id", "M1-S16-U16");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_ready returning void does not satisfy "
      "convertible_to<bool>");

  EXPECT_FALSE(ropic::safe_awaitable<ReadyReturnsVoid>);
}

TEST(M1S16SafeAwaitable, U17OverloadedTemplateSuspendRejected)
{
  RecordProperty("id", "M1-S16-U17");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Overloaded await_suspend is ambiguous; template cannot be "
      "addressed via &T::await_suspend");

  EXPECT_FALSE(ropic::safe_awaitable<OverloadedSuspend>);
  EXPECT_FALSE(ropic::safe_awaitable<TemplateSuspend>);
}

TEST(M1S16SafeAwaitable, U18ConstRefNonConstRSRejected)
{
  RecordProperty("id", "M1-S16-U18");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(const ValidRS&) fails: non-const requestResume "
      "cannot be called on const ref");

  EXPECT_FALSE(ropic::safe_awaitable<SuspendConstRefNonConstRS>);
}

TEST(M1S16SafeAwaitable, U19MemberCoAwaitStdRejected)
{
  RecordProperty("id", "M1-S16-U19");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member operator co_await() returning standard awaiter rejected "
      "by safe_awaitable");

  EXPECT_FALSE(ropic::safe_awaitable<MemberCoAwtStd>);
}

TEST(M1S16SafeAwaitable, U20NonMemberCoAwaitStdRejected)
{
  RecordProperty("id", "M1-S16-U20");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Free operator co_await(T) returning standard awaiter rejected "
      "by safe_awaitable");

  EXPECT_FALSE(ropic::safe_awaitable<NonMemberTargetStd>);
}

// NOLINTEND(readability-magic-numbers)
