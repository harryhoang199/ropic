// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <coroutine>
#include <string>

#include "ropic/detail/either/resume_source.hpp"

// NOLINTBEGIN(readability-identifier-naming,readability-convert-member-functions-to-static)

/// @name resume_source concept fixtures
/// Minimal types satisfying or violating the `resume_source` concept.
/// @{

// =============================================================================
// resume_source fixtures
// =============================================================================

struct ValidRS
{
  void requestResume();
};

struct RSConst
{
  void requestResume() const;
};

/// @}

/// @name safe_awaiter concept fixtures (positive)
/// Types that satisfy `safe_awaiter`: their `await_suspend` takes a
/// `resume_source`-compatible parameter.
/// @{

// =============================================================================
// safe_awaiter fixtures (positive)
// =============================================================================

struct MinSafeAwaiter
{
  auto await_ready() -> bool;
  void await_suspend(ValidRS);
  void await_resume();
};

struct SuspendTakesResumeSource
{
  auto await_ready() -> bool;
  void await_suspend(ropic::ResumeSource);
  auto await_resume() -> std::string;
};

struct VariedSignatures
{
  auto await_ready() -> int;
  auto await_suspend(ValidRS) -> bool;
  auto await_resume() -> int;
};

struct SuspendTakesRefRS
{
  auto await_ready() -> bool;
  void await_suspend(ValidRS&);
  void await_resume();
};

struct SuspendTakesConstRef
{
  auto await_ready() -> bool;
  void await_suspend(const RSConst&);
  void await_resume();
};

struct NoexceptAwaiter
{
  auto await_ready() noexcept -> bool;
  void await_suspend(ValidRS) noexcept;
  void await_resume() noexcept;
};

struct SuspendTakesRvalueRefRS
{
  auto await_ready() -> bool;
  void await_suspend(ValidRS&&);
  void await_resume();
};

/// @}

/// @name co_awaitable fixtures (positive)
/// Types with `operator co_await()` returning a `safe_awaiter`.
/// @{

// =============================================================================
// co_awaitable fixtures (positive)
// =============================================================================

struct MemberCoAwt
{
  auto operator co_await() -> MinSafeAwaiter { return {}; }
};

struct NonMemberTarget
{
};

inline auto operator co_await(NonMemberTarget) -> MinSafeAwaiter { return {}; }

struct BothOperators
{
  auto operator co_await() -> MinSafeAwaiter { return {}; }
};

inline auto operator co_await(BothOperators) -> MinSafeAwaiter { return {}; }

/// @}

/// @name Negative fixtures (standard awaiters / non-safe)
/// Types that do NOT satisfy `safe_awaiter` or `safe_awaitable` — their
/// `await_suspend` takes `std::coroutine_handle<>` instead of a
/// `resume_source`.
/// @{

// =============================================================================
// negative fixtures (standard awaiters / non-safe)
// =============================================================================

struct StandardAwaiter
{
  auto await_ready() -> bool;
  void await_suspend(std::coroutine_handle<>);
  void await_resume();
};

struct MemberCoAwtStd
{
  auto operator co_await() -> StandardAwaiter { return {}; }
};

struct NonMemberTargetStd
{
};

inline auto operator co_await(NonMemberTargetStd) -> StandardAwaiter
{
  return {};
}

// ---- co_awaitable returning cv-ref awaiter ----

struct MemberCoAwtReturnsRef
{
  MinSafeAwaiter _inner;
  auto operator co_await() -> MinSafeAwaiter& { return _inner; }
};

struct MemberCoAwtReturnsConstRef
{
  MinSafeAwaiter _inner;
  auto operator co_await() const -> const MinSafeAwaiter& { return _inner; }
};

struct NonMemberTargetReturnsRef
{
};

inline auto operator co_await(NonMemberTargetReturnsRef) -> MinSafeAwaiter&;

// ---- ResumeSource integration fixtures ----

struct SuspendTakesConstRefResumeSource
{
  auto await_ready() -> bool;
  void await_suspend(const ropic::ResumeSource&);
  auto await_resume() -> int;
};

struct SuspendTakesRvalueRefResumeSource
{
  auto await_ready() -> bool;
  void await_suspend(ropic::ResumeSource&&);
  auto await_resume() -> double;
};

struct MemberCoAwtRS
{
  auto operator co_await() -> SuspendTakesResumeSource { return {}; }
};

struct NonMemberTargetRS
{
};

inline auto operator co_await(NonMemberTargetRS) -> SuspendTakesResumeSource
{
  return {};
}
// NOLINTEND(readability-identifier-naming,readability-convert-member-functions-to-static)
