// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include <string>

#include "utils/SafeAwaitableFixtures.hpp"

#include "ropic/detail/either/resume_source.hpp"
#include "ropic/detail/shared/safe_awaiter_traits.hpp"

// NOLINTBEGIN(readability-identifier-naming,readability-convert-member-functions-to-static)
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-function"
#  pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif
namespace
{
// ---- specialization priority fixture ----

struct AllThreePaths
{
  auto await_ready() -> bool;
  void await_suspend(ValidRS);
  auto await_resume() -> int;
  auto operator co_await() -> MinSafeAwaiter { return {}; }
};

auto operator co_await(AllThreePaths) -> MinSafeAwaiter { return {}; }

} // namespace
// NOLINTEND(readability-identifier-naming,readability-convert-member-functions-to-static)
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

// NOLINTBEGIN(readability-magic-numbers)

// =============================================================================
// Part A: SafeAwaiterTraits — Direct safe_awaiter Path (U01-U07)
// =============================================================================

TEST(M1S17SafeAwaiterTraits, U01MinimalSafeAwaiterTypes)
{
  RecordProperty("id", "M1-S17-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "SafeAwaiterTraits extracts correct types from minimal awaiter "
      "with by-value ValidRS");

  using T = ropic::SafeAwaiterTraits<MinSafeAwaiter>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U02VariedReturnTypes)
{
  RecordProperty("id", "M1-S17-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "SafeAwaiterTraits extracts bool SuspendReturnType and int "
      "ResumeReturnType");

  using T = ropic::SafeAwaiterTraits<VariedSignatures>;
  EXPECT_TRUE((std::same_as<T::Awaiter, VariedSignatures>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, bool>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, int>));
}

TEST(M1S17SafeAwaiterTraits, U03ByRefSuspendParam)
{
  RecordProperty("id", "M1-S17-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "SuspendArgType preserves lvalue reference qualifier "
      "(ValidRS&)");

  using T = ropic::SafeAwaiterTraits<SuspendTakesRefRS>;
  EXPECT_TRUE((std::same_as<T::Awaiter, SuspendTakesRefRS>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS&>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U04ConstRefSuspendParam)
{
  RecordProperty("id", "M1-S17-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "SuspendArgType preserves const lvalue reference qualifier "
      "(const RSConst&)");

  using T = ropic::SafeAwaiterTraits<SuspendTakesConstRef>;
  EXPECT_TRUE((std::same_as<T::Awaiter, SuspendTakesConstRef>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, const RSConst&>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U05NoexceptDoesNotChangeTypes)
{
  RecordProperty("id", "M1-S17-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "noexcept qualification does not affect extracted type aliases");

  using T = ropic::SafeAwaiterTraits<NoexceptAwaiter>;
  EXPECT_TRUE((std::same_as<T::Awaiter, NoexceptAwaiter>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U06RvalueRefSuspendParam)
{
  RecordProperty("id", "M1-S17-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "SuspendArgType preserves rvalue reference qualifier "
      "(ValidRS&&)");

  using T = ropic::SafeAwaiterTraits<SuspendTakesRvalueRefRS>;
  EXPECT_TRUE((std::same_as<T::Awaiter, SuspendTakesRvalueRefRS>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS&&>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U07DirectResumeSourceTypes)
{
  RecordProperty("id", "M1-S17-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "SafeAwaiterTraits extracts ropic::ResumeSource as "
      "SuspendArgType and std::string as ResumeReturnType");

  using T = ropic::SafeAwaiterTraits<SuspendTakesResumeSource>;
  EXPECT_TRUE((std::same_as<T::Awaiter, SuspendTakesResumeSource>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ropic::ResumeSource>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, std::string>));
}

// =============================================================================
// Part B: SafeAwaiterTraits — co_awaitable Path (U08-U14)
// =============================================================================

TEST(M1S17SafeAwaiterTraits, U08MemberCoAwaitDelegates)
{
  RecordProperty("id", "M1-S17-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member operator co_await path: Awaiter alias is inner "
      "MinSafeAwaiter, not outer MemberCoAwt");

  using T = ropic::SafeAwaiterTraits<MemberCoAwt>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U09NonMemberCoAwaitDelegates)
{
  RecordProperty("id", "M1-S17-U09");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Non-member operator co_await path: Awaiter alias is inner "
      "MinSafeAwaiter, not outer NonMemberTarget");

  using T = ropic::SafeAwaiterTraits<NonMemberTarget>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U10BothOperatorsMemberWins)
{
  RecordProperty("id", "M1-S17-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Type with both member and non-member operator co_await: "
      "member path selected, Awaiter is MinSafeAwaiter");

  using T = ropic::SafeAwaiterTraits<BothOperators>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U11AllThreePathsDirectWins)
{
  RecordProperty("id", "M1-S17-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Type satisfying safe_awaiter + member_co_awaitable + "
      "non_member_co_awaitable: direct path wins; "
      "ResumeReturnType is int (not void from inner)");

  using T = ropic::SafeAwaiterTraits<AllThreePaths>;
  // Awaiter is AllThreePaths itself (direct path),
  // NOT MinSafeAwaiter (co_await path)
  EXPECT_TRUE((std::same_as<T::Awaiter, AllThreePaths>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  // int proves direct path was selected
  // (inner MinSafeAwaiter returns void)
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, int>));
}

TEST(M1S17SafeAwaiterTraits, U12MemberCoAwaitReturnsRef)
{
  RecordProperty("id", "M1-S17-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member operator co_await returns MinSafeAwaiter&: Awaiter "
      "alias is MinSafeAwaiter& (ref preserved)");

  using T = ropic::SafeAwaiterTraits<MemberCoAwtReturnsRef>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter&>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U13MemberCoAwaitReturnsConstRef)
{
  RecordProperty("id", "M1-S17-U13");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member operator co_await returns const MinSafeAwaiter&: "
      "Awaiter alias is const MinSafeAwaiter& (const-ref preserved)");

  using T = ropic::SafeAwaiterTraits<MemberCoAwtReturnsConstRef>;
  EXPECT_TRUE((std::same_as<T::Awaiter, const MinSafeAwaiter&>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U14NonMemberCoAwaitReturnsRef)
{
  RecordProperty("id", "M1-S17-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Free operator co_await returns MinSafeAwaiter&: Awaiter alias "
      "is MinSafeAwaiter& (ref preserved through non-member path)");

  using T = ropic::SafeAwaiterTraits<NonMemberTargetReturnsRef>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter&>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

// =============================================================================
// Part C: SafeAwaiterTraits — Integration with ropic::ResumeSource (U15-U18)
// =============================================================================

TEST(M1S17SafeAwaiterTraits, U15ConstRefResumeSource)
{
  RecordProperty("id", "M1-S17-U15");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "SuspendArgType is const ropic::ResumeSource& with int "
      "ResumeReturnType");

  using T = ropic::SafeAwaiterTraits<SuspendTakesConstRefResumeSource>;
  EXPECT_TRUE((std::same_as<T::Awaiter, SuspendTakesConstRefResumeSource>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, const ropic::ResumeSource&>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, int>));
}

TEST(M1S17SafeAwaiterTraits, U16RvalueRefResumeSource)
{
  RecordProperty("id", "M1-S17-U16");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "SuspendArgType is ropic::ResumeSource&& with double "
      "ResumeReturnType");

  using T = ropic::SafeAwaiterTraits<SuspendTakesRvalueRefResumeSource>;
  EXPECT_TRUE((std::same_as<T::Awaiter, SuspendTakesRvalueRefResumeSource>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ropic::ResumeSource&&>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, double>));
}

TEST(M1S17SafeAwaiterTraits, U17MemberCoAwaitWithResumeSource)
{
  RecordProperty("id", "M1-S17-U17");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Member co_await delegates to SuspendTakesResumeSource; "
      "SuspendArgType is ropic::ResumeSource");

  using T = ropic::SafeAwaiterTraits<MemberCoAwtRS>;
  EXPECT_TRUE((std::same_as<T::Awaiter, SuspendTakesResumeSource>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ropic::ResumeSource>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, std::string>));
}

TEST(M1S17SafeAwaiterTraits, U18NonMemberCoAwaitWithResumeSource)
{
  RecordProperty("id", "M1-S17-U18");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Non-member co_await delegates to SuspendTakesResumeSource; "
      "SuspendArgType is ropic::ResumeSource");

  using T = ropic::SafeAwaiterTraits<NonMemberTargetRS>;
  EXPECT_TRUE((std::same_as<T::Awaiter, SuspendTakesResumeSource>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ropic::ResumeSource>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, std::string>));
}

// =============================================================================
// Part D: SafeAwaiterTraits — cv-ref Qualified Template Argument (U19-U24)
// =============================================================================

TEST(M1S17SafeAwaiterTraits, U19LvalueRefQualifiedA)
{
  RecordProperty("id", "M1-S17-U19");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Awaiter alias preserves lvalue-ref; other aliases invariant "
      "under ref qualification");

  using T = ropic::SafeAwaiterTraits<MinSafeAwaiter&>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter&>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U20RvalueRefQualifiedA)
{
  RecordProperty("id", "M1-S17-U20");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Awaiter alias preserves rvalue-ref; other aliases invariant "
      "under ref qualification");

  using T = ropic::SafeAwaiterTraits<MinSafeAwaiter&&>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter&&>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U21ConstLvalueRefQualifiedA)
{
  RecordProperty("id", "M1-S17-U21");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Awaiter alias preserves const lvalue-ref; other aliases "
      "invariant under cv-ref qualification");

  using T = ropic::SafeAwaiterTraits<const MinSafeAwaiter&>;
  EXPECT_TRUE((std::same_as<T::Awaiter, const MinSafeAwaiter&>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U22ConstRvalueRefQualifiedA)
{
  RecordProperty("id", "M1-S17-U22");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Awaiter alias preserves const rvalue-ref; other aliases "
      "invariant under cv-ref qualification");

  using T = ropic::SafeAwaiterTraits<const MinSafeAwaiter&&>;
  EXPECT_TRUE((std::same_as<T::Awaiter, const MinSafeAwaiter&&>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U23LvalueRefCoAwaitable)
{
  RecordProperty("id", "M1-S17-U23");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_awaitable with lvalue-ref A: Awaiter is inner "
      "MinSafeAwaiter, ref on A has no effect");

  using T = ropic::SafeAwaiterTraits<MemberCoAwt&>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

TEST(M1S17SafeAwaiterTraits, U24ConstLvalueRefCoAwaitable)
{
  RecordProperty("id", "M1-S17-U24");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_awaitable with const lvalue-ref A: Awaiter is inner "
      "MinSafeAwaiter, cv-ref on A has no effect");

  using T = ropic::SafeAwaiterTraits<const MemberCoAwt&>;
  EXPECT_TRUE((std::same_as<T::Awaiter, MinSafeAwaiter>));
  EXPECT_TRUE((std::same_as<T::SuspendArgType, ValidRS>));
  EXPECT_TRUE((std::same_as<T::SuspendReturnType, void>));
  EXPECT_TRUE((std::same_as<T::ResumeReturnType, void>));
}

// NOLINTEND(readability-magic-numbers)
