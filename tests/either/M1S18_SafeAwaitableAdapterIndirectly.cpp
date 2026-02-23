// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include <ropic.hpp>
#include <string>

#include "utils/SafeAdapterFixtures.hpp"

// NOLINTBEGIN(readability-magic-numbers)
using namespace ropic;

// =============================================================================
// Part A: Inline Resume — Direct safe_awaiter (U01-U08)
// =============================================================================

TEST(M1S18SafeAwaitableAdapterIndirectly, U01InlineVoidCoReturnValue)
{
  RecordProperty("id", "M1-S18-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await void-returning safe awaiter with inline resume, "
      "then co_return int value");

  auto coro = []() -> Either<int, std::string>
  {
    co_await VoidResumeAwaiter{.mode = ResumeMode::INLINE};
    co_return 42;
  };
  auto result = coro();
  EXPECT_EQ(result.state(), CoroState::READY);
  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U02InlineIntResult)
{
  RecordProperty("id", "M1-S18-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await int-returning safe awaiter with inline resume "
      "propagates value through adapter");

  auto coro = []() -> Either<int, std::string>
  {
    co_return co_await ValueResumeAwaiter<int>{
        .value = 42, .mode = ResumeMode::INLINE};
  };

  auto result = coro();

  EXPECT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U03InlineStringResult)
{
  RecordProperty("id", "M1-S18-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await string-returning safe awaiter with inline resume "
      "propagates string value");

  auto coro = []() -> Either<std::string, int>
  {
    co_return co_await ValueResumeAwaiter<std::string>{
        .value = "hello", .mode = ResumeMode::INLINE};
  };

  auto result = coro();

  EXPECT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), "hello");
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U04AwaitReadyTrueNoSuspension)
{
  RecordProperty("id", "M1-S18-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_ready returns true: no suspension occurs, "
      "Either completes immediately with DONE");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await ReadyAwaiter<int>{42}; };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::DONE);

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U05BoolTrueSuspends)
{
  RecordProperty("id", "M1-S18-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend returns true: coroutine suspends, "
      "resume() completes it");

  auto coro = []() -> Either<int, std::string>
  {
    co_return co_await BoolSuspendAwaiter<int>{
        .value = 99, .shouldSuspend = true};
  };

  auto result = coro();

  EXPECT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 99);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U06BoolFalseNoSuspension)
{
  RecordProperty("id", "M1-S18-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend returns false: coroutine not suspended, "
      "completes immediately");

  auto coro = []() -> Either<int, std::string>
  {
    co_return co_await BoolSuspendAwaiter<int>{
        .value = 99, .shouldSuspend = false};
  };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::DONE);

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 99);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U07TwoSequentialInlineCoAwaits)
{
  RecordProperty("id", "M1-S18-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Two sequential co_await safe awaitables in one coroutine; "
      "two resume() calls; value is sum");

  auto coro = []() -> Either<int, std::string>
  {
    int a = co_await ValueResumeAwaiter<int>{
        .value = 10, .mode = ResumeMode::INLINE};
    int b = co_await ValueResumeAwaiter<int>{
        .value = 20, .mode = ResumeMode::INLINE};
    co_return a + b;
  };

  auto result = coro();

  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 30);
}

TEST(
    M1S18SafeAwaitableAdapterIndirectly, U08HandleReturnInlineSymmetricTransfer)
{
  RecordProperty("id", "M1-S18-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend returns non-Either coroutine_handle: "
      "symmetric transfer occurs, inline resume propagates value");

  auto helper = makeHelperCoro();
  ASSERT_FALSE(helper.handle.promise().finished);

  auto coro = [&helper]() -> Either<int, std::string>
  {
    co_return co_await HandleReturnAwaiter<int>{
        .value = 42,
        .mode = ResumeMode::INLINE,
        .transferTarget = helper.handle};
  };

  auto result = coro();

  ASSERT_TRUE(helper.handle.promise().finished);

  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

// =============================================================================
// Part B: Async Resume — State Transitions (U09-U15)
// =============================================================================

TEST(M1S18SafeAwaitableAdapterIndirectly, U09AsyncVoidStateTransitions)
{
  RecordProperty("id", "M1-S18-U09");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Async resume: state is PENDING after create, "
      "resume() blocks until requestResume, then DONE");

  auto coro = []() -> Either<int, std::string>
  {
    co_await VoidResumeAwaiter{.mode = ResumeMode::ASYNC};
    co_return 42;
  };

  auto result = coro();

  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U10AsyncIntValue)
{
  RecordProperty("id", "M1-S18-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Async resume from background thread propagates "
      "int value correctly");

  auto coro = []() -> Either<int, std::string>
  {
    co_return co_await ValueResumeAwaiter<int>{
        .value = 42, .mode = ResumeMode::ASYNC};
  };

  auto result = coro();

  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U11AsyncStringValue)
{
  RecordProperty("id", "M1-S18-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Async resume from background thread propagates "
      "string value correctly");

  auto coro = []() -> Either<std::string, int>
  {
    co_return co_await ValueResumeAwaiter<std::string>{
        .value = "async", .mode = ResumeMode::ASYNC};
  };

  auto result = coro();

  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), "async");
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U12TwoSequentialAsyncCoAwaits)
{
  RecordProperty("id", "M1-S18-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Two sequential co_await with ASYNC resume in one coroutine; "
      "two resume() calls; value is sum");

  auto coro = []() -> Either<int, std::string>
  {
    int a = co_await ValueResumeAwaiter<int>{
        .value = 10, .mode = ResumeMode::ASYNC};
    int b = co_await ValueResumeAwaiter<int>{
        .value = 20, .mode = ResumeMode::ASYNC};
    co_return a + b;
  };

  auto result = coro();
  result.resume();
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 30);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U13MixedAsyncThenInline)
{
  RecordProperty("id", "M1-S18-U13");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "First co_await ASYNC, second co_await INLINE "
      "in one coroutine; two resume() calls");

  auto coro = []() -> Either<int, std::string>
  {
    int a = co_await ValueResumeAwaiter<int>{
        .value = 10, .mode = ResumeMode::ASYNC};
    int b = co_await ValueResumeAwaiter<int>{
        .value = 20, .mode = ResumeMode::INLINE};
    co_return a + b;
  };

  auto result = coro();
  result.resume();

  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 30);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U14AsyncBoolTrueResume)
{
  RecordProperty("id", "M1-S18-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend returns bool(true) with async requestResume: "
      "functionally equivalent to void return");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await AsyncBoolTrueAwaiter<int>{.value = 99}; };

  auto result = coro();
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 99);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U15AsyncHandleReturnSymmetricTransfer)
{
  RecordProperty("id", "M1-S18-U15");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend returns non-Either coroutine_handle with "
      "async requestResume: symmetric transfer + async resume");

  auto helper = makeHelperCoro();
  ASSERT_FALSE(helper.handle.promise().finished);

  auto coro = [&helper]() -> Either<int, std::string>
  {
    co_return co_await HandleReturnAwaiter<int>{
        .value = 42,
        .mode = ResumeMode::ASYNC,
        .transferTarget = helper.handle};
  };

  auto result = coro();
  ASSERT_TRUE(helper.handle.promise().finished);

  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

// =============================================================================
// Part C: ResumeSource Parameter Passing Modes (U16-U17)
// =============================================================================

TEST(M1S18SafeAwaitableAdapterIndirectly, U16ConstRefResumeSource)
{
  RecordProperty("id", "M1-S18-U16");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(const ResumeSource&): "
      "temporary binds to const-ref, requestResume is const");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await ConstRefResumeAwaiter<int>{.value = 42}; };

  auto result = coro();
  result.resume();

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U17RvalueRefResumeSource)
{
  RecordProperty("id", "M1-S18-U17");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(ResumeSource&&): "
      "temporary binds to rvalue-ref, inline resume works");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await RvalueRefResumeAwaiter<int>{.value = 42}; };

  auto result = coro();
  result.resume();

  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

// =============================================================================
// Part D: co_awaitable Path (U18-U21)
// =============================================================================

TEST(M1S18SafeAwaitableAdapterIndirectly, U18MemberCoAwaitInline)
{
  RecordProperty("id", "M1-S18-U18");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member operator co_await returns awaiter by value; "
      "inline resume propagates value");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await MemberCoAwtValue{.value = 42}; };

  auto result = coro();

  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U19NonMemberCoAwaitInline)
{
  RecordProperty("id", "M1-S18-U19");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Non-member operator co_await returns awaiter by value; "
      "inline resume propagates value");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await NonMemberValueTarget{.value = 42}; };

  auto result = coro();

  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U20MemberCoAwaitAsync)
{
  RecordProperty("id", "M1-S18-U20");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member operator co_await with async resume; "
      "resume() blocks until complete");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await MemberCoAwtAsync{.value = 42}; };

  auto result = coro();
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U21NonMemberCoAwaitAsync)
{
  RecordProperty("id", "M1-S18-U21");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Non-member operator co_await with async resume; "
      "resume() blocks until complete");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await NonMemberAsyncTarget{.value = 42}; };

  auto result = coro();
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

// =============================================================================
// Part E: Reference Preservation through co_await (U22-U26)
// =============================================================================

TEST(M1S18SafeAwaitableAdapterIndirectly, U22DirectLvalueCoAwaitRefPreserved)
{
  RecordProperty("id", "M1-S18-U22");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await lvalue TrackedAwaiter directly: adapter deduces "
      "SAFE_AWT=TrackedAwaiter&, stores reference");

  TrackedAwaiter trackedAwaiter;
  trackedAwaiter.value = 42;

  auto coro = [&trackedAwaiter]() -> Either<int, std::string>
  { co_return co_await trackedAwaiter; };

  auto result = coro();
  result.resume();

  EXPECT_TRUE(trackedAwaiter.suspendCalled);

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U23MemberCoAwaitRefSideEffect)
{
  RecordProperty("id", "M1-S18-U23");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member co_await returns TrackedAwaiter&: "
      "suspendCalled on original proves adapter stores reference");

  TrackedMemberCoAwtRef awter;
  awter.inner.value = 42;

  auto coro = [&awter]() -> Either<int, std::string>
  { co_return co_await awter; };

  auto result = coro();
  result.resume();

  EXPECT_TRUE(awter.inner.suspendCalled);

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U24MemberCoAwaitCopyBaseline)
{
  RecordProperty("id", "M1-S18-U24");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member co_await returns TrackedAwaiter by value: "
      "original suspendCalled remains false (copy baseline)");

  TrackedMemberCoAwtCopy awter;
  awter.inner.value = 42;

  auto coro = [&awter]() -> Either<int, std::string>
  { co_return co_await awter; };

  auto result = coro();
  result.resume();

  EXPECT_FALSE(awter.inner.suspendCalled);

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U25NonMemberCoAwaitRefSideEffect)
{
  RecordProperty("id", "M1-S18-U25");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Non-member co_await returns TrackedAwaiter&: "
      "suspendCalled on original proves reference");

  NonMemberRefTarget awter;
  awter.inner.value = 42;

  auto coro = [&awter]() -> Either<int, std::string>
  { co_return co_await awter; };

  auto result = coro();
  result.resume();

  EXPECT_TRUE(awter.inner.suspendCalled);

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(
    M1S18SafeAwaitableAdapterIndirectly, U26ReferenceLivenessModifyBeforeResume)
{
  RecordProperty("id", "M1-S18-U26");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Reference is live: modify inner.value after coroutine "
      "creation but before resume; new value returned");

  TrackedMemberCoAwtRef awter;
  awter.inner.value = 42;

  auto coro = [&awter]() -> Either<int, std::string>
  { co_return co_await awter; };

  auto result = coro();

  // Modify AFTER coroutine suspended but BEFORE resume
  awter.inner.value = 99;
  result.resume();

  ASSERT_EQ(result.state(), CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 99);
}

// =============================================================================
// Part F: Custom resume_source Integration (U27-U34)
// =============================================================================

TEST(M1S18SafeAwaitableAdapterIndirectly, U27CustomRSInlineVoid)
{
  RecordProperty("id", "M1-S18-U27");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "CustomResumeSource by value: inline void-returning awaiter "
      "integrates through Either");

  auto coro = []() -> Either<int, std::string>
  {
    co_await CustomVoidResumeAwaiter{.mode = ResumeMode::INLINE};
    co_return 1;
  };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U28CustomRSAsyncValue)
{
  RecordProperty("id", "M1-S18-U28");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "CustomResumeSource by value: async value-returning awaiter "
      "integrates through Either");

  auto coro = []() -> Either<int, std::string>
  {
    co_return co_await CustomValueResumeAwaiter<int>{
        .value = 1, .mode = ResumeMode::ASYNC};
  };

  auto result = coro();
  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U29CustomRSConstRef)
{
  RecordProperty("id", "M1-S18-U29");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(const CustomResumeSource&): "
      "temporary binds to const-ref, integrates through Either");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await CustomConstRefResumeAwaiter<int>{.value = 1}; };

  auto result = coro();
  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U30CustomRSRvalueRef)
{
  RecordProperty("id", "M1-S18-U30");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "await_suspend(CustomResumeSource&&): "
      "temporary binds to rvalue-ref, integrates through Either");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await CustomRvalueRefResumeAwaiter<int>{.value = 1}; };

  auto result = coro();
  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U31CustomRSMemberCoAwait)
{
  RecordProperty("id", "M1-S18-U31");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member operator co_await returning CustomResumeSource awaiter "
      "integrates through Either");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await CustomMemberCoAwtValue{.value = 1}; };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
}

TEST(M1S18SafeAwaitableAdapterIndirectly, U32CustomRSNonMemberCoAwait)
{
  RecordProperty("id", "M1-S18-U32");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Non-member operator co_await returning CustomResumeSource "
      "awaiter integrates through Either");

  auto coro = []() -> Either<int, std::string>
  { co_return co_await CustomNonMemberValueTarget{.value = 1}; };

  auto result = coro();
  ASSERT_EQ(result.state(), CoroState::READY);
  result.resume();
  ASSERT_EQ(result.state(), CoroState::DONE);
}

// NOLINTEND(readability-magic-numbers)
