// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <coroutine>
#include <cstdint>
#include <thread>
#include <utility>

#include "ropic/detail/either/resume_source.hpp"

// NOLINTBEGIN(readability-identifier-naming,readability-convert-member-functions-to-static)

// =============================================================================
// Resume mode
// =============================================================================

/// @brief Controls how a test awaiter signals resumption.
enum class ResumeMode : std::uint8_t
{
  INLINE, ///< Signal resume synchronously within await_suspend.
  ASYNC   ///< Signal resume from a detached background thread.
};

// =============================================================================
// Direct safe_awaiter fixtures (ropic::ResumeSource by value)
// =============================================================================

/// Void result, configurable resume mode.
struct VoidResumeAwaiter
{
  ResumeMode mode;

  auto await_ready() -> bool { return false; }

  void await_suspend(ropic::ResumeSource rs) const
  {
    if (mode == ResumeMode::INLINE)
      rs.requestResume();
    else
      std::thread([s = std::move(rs)]() mutable { s.requestResume(); })
          .detach();
  }

  void await_resume() {}
};

/// Typed result, configurable resume mode.
template <typename T>
struct ValueResumeAwaiter
{
  T value;
  ResumeMode mode;

  auto await_ready() -> bool { return false; }

  void await_suspend(ropic::ResumeSource rs)
  {
    if (mode == ResumeMode::INLINE)
      rs.requestResume();
    else
      std::thread([s = std::move(rs)]() mutable { s.requestResume(); })
          .detach();
  }

  auto await_resume() -> T { return std::move(value); }
};

/// Already ready — never suspends.
template <typename T>
struct ReadyAwaiter
{
  T value;

  auto await_ready() -> bool { return true; }
  void await_suspend(ropic::ResumeSource) {}
  auto await_resume() -> T { return std::move(value); }
};

/// await_suspend returns bool: true -> suspend, false -> no suspend.
template <typename T>
struct BoolSuspendAwaiter
{
  T value;
  bool shouldSuspend;

  auto await_ready() -> bool { return false; }

  auto await_suspend(ropic::ResumeSource rs) -> bool
  {
    if (shouldSuspend)
    {
      rs.requestResume();
      return true;
    }
    return false;
  }

  auto await_resume() -> T { return std::move(value); }
};

/// await_suspend returns bool(true) with ASYNC requestResume.
/// Always suspends; resume is signaled from a background thread.
template <typename T>
struct AsyncBoolTrueAwaiter
{
  T value;

  auto await_ready() -> bool { return false; }

  auto await_suspend(ropic::ResumeSource rs) -> bool
  {
    std::thread([s = std::move(rs)]() mutable { s.requestResume(); }).detach();
    return true;
  }

  auto await_resume() -> T { return std::move(value); }
};

// =============================================================================
// Symmetric transfer fixtures (await_suspend returns coroutine_handle)
// =============================================================================

/// Minimal non-Either coroutine for symmetric transfer tests.
/// Body does nothing; exists solely to provide a valid coroutine_handle.
struct HelperCoro
{
  struct promise_type
  {
    bool finished = false;
    auto get_return_object() -> HelperCoro
    {
      return HelperCoro{
          std::coroutine_handle<promise_type>::from_promise(*this)};
    }
    auto initial_suspend() noexcept -> std::suspend_always { return {}; }
    auto final_suspend() noexcept -> std::suspend_always
    {
      finished = true;
      return {};
    }
    void return_void() {}
    void unhandled_exception() { std::terminate(); }
  };

  std::coroutine_handle<promise_type> handle;

  explicit HelperCoro(std::coroutine_handle<promise_type> h)
      : handle(h)
  {
  }
  HelperCoro(HelperCoro&& o) noexcept
      : handle(o.handle)
  {
    o.handle = nullptr;
  }
  auto operator=(HelperCoro&&) -> HelperCoro& = delete;
  HelperCoro(const HelperCoro&) = delete;
  auto operator=(const HelperCoro&) -> HelperCoro& = delete;

  ~HelperCoro()
  {
    if (handle)
      handle.destroy();
  }
};

/// @brief Factory that produces a suspended HelperCoro for tests.
inline auto makeHelperCoro() -> HelperCoro { co_return; }

/// await_suspend returns std::coroutine_handle<> for symmetric transfer.
/// Configurable resume mode. transferTarget is the coroutine to transfer to.
template <typename T>
struct HandleReturnAwaiter
{
  T value;
  ResumeMode mode;
  std::coroutine_handle<> transferTarget;

  auto await_ready() -> bool { return false; }

  auto await_suspend(ropic::ResumeSource rs) -> std::coroutine_handle<>
  {
    if (mode == ResumeMode::INLINE)
      rs.requestResume();
    else
      std::thread([s = std::move(rs)]() mutable { s.requestResume(); })
          .detach();
    return transferTarget;
  }

  auto await_resume() -> T { return std::move(value); }
};

// =============================================================================
// ResumeSource parameter passing mode fixtures
// =============================================================================

/// await_suspend takes const ropic::ResumeSource&.
template <typename T>
struct ConstRefResumeAwaiter
{
  T value;

  auto await_ready() -> bool { return false; }

  void await_suspend(const ropic::ResumeSource& rs) { rs.requestResume(); }

  auto await_resume() -> T { return std::move(value); }
};

/// await_suspend takes ropic::ResumeSource&&.
template <typename T>
struct RvalueRefResumeAwaiter
{
  T value;

  auto await_ready() -> bool { return false; }

  void await_suspend(ropic::ResumeSource&& rs) { rs.requestResume(); }

  auto await_resume() -> T { return std::move(value); }
};

// =============================================================================
// Tracked awaiter for reference preservation tests
// =============================================================================

/// Tracks whether await_suspend was called on THIS instance.
/// Used to distinguish reference storage from copy storage.
struct TrackedAwaiter
{
  bool suspendCalled = false;
  int value = 0;

  auto await_ready() -> bool { return false; }

  void await_suspend(ropic::ResumeSource rs)
  {
    suspendCalled = true;
    rs.requestResume();
  }

  [[nodiscard]]
  auto await_resume() const -> int
  {
    return value;
  }
};

// =============================================================================
// co_awaitable fixtures — member operator co_await
// =============================================================================

/// Member co_await returning awaiter by VALUE, inline resume.
struct MemberCoAwtValue
{
  int value;
  auto operator co_await() -> ValueResumeAwaiter<int>
  {
    return {.value = value, .mode = ResumeMode::INLINE};
  }
};

/// Member co_await returning awaiter by VALUE, async resume.
struct MemberCoAwtAsync
{
  int value;
  auto operator co_await() -> ValueResumeAwaiter<int>
  {
    return {.value = value, .mode = ResumeMode::ASYNC};
  }
};

/// Member co_await returning LVALUE REF to inner TrackedAwaiter.
/// Adapter stores TrackedAwaiter& — mutations visible through reference.
/// (Renamed from MemberCoAwtReturnsRef to avoid collision with
///  SafeAwaitableFixtures.hpp)
struct TrackedMemberCoAwtRef
{
  TrackedAwaiter inner;
  auto operator co_await() -> TrackedAwaiter& { return inner; }
};

/// Member co_await returning TrackedAwaiter by VALUE (copy).
/// Adapter stores TrackedAwaiter — original NOT mutated.
/// (Renamed from MemberCoAwtReturnsCopy to avoid collision)
struct TrackedMemberCoAwtCopy
{
  TrackedAwaiter inner;
  auto operator co_await() const -> TrackedAwaiter { return inner; }
};

// =============================================================================
// co_awaitable fixtures — non-member operator co_await
// =============================================================================

/// Non-member co_await target, inline resume.
struct NonMemberValueTarget
{
  int value;
};

inline auto operator co_await(NonMemberValueTarget t) -> ValueResumeAwaiter<int>
{
  return {.value = t.value, .mode = ResumeMode::INLINE};
}

/// Non-member co_await target, async resume.
struct NonMemberAsyncTarget
{
  int value;
};

inline auto operator co_await(NonMemberAsyncTarget t) -> ValueResumeAwaiter<int>
{
  return {.value = t.value, .mode = ResumeMode::ASYNC};
}

/// Non-member co_await returning LVALUE REF to inner TrackedAwaiter.
struct NonMemberRefTarget
{
  TrackedAwaiter inner;
};

inline auto operator co_await(NonMemberRefTarget& t) -> TrackedAwaiter&
{
  return t.inner;
}

// =============================================================================
// Custom resume_source — user-defined type (not ropic::ResumeSource)
// =============================================================================

/// User-defined resume source satisfying the resume_source concept.
/// Mirrors ropic::ResumeSource interface but is a distinct type,
/// proving SafeAwaitableAdapter works with any conforming resume_source.
struct CustomResumeSource
{
  std::atomic<ropic::detail::ResumePhase>* state;

  CustomResumeSource(std::atomic<ropic::detail::ResumePhase>& s)
      : state(&s)
  {
  }

  CustomResumeSource(const CustomResumeSource&) = default;
  auto operator=(const CustomResumeSource&)
      -> CustomResumeSource& = default;

  CustomResumeSource(CustomResumeSource&& other) noexcept
      : state(other.state)
  {
    other.state = nullptr;
  }

  auto operator=(CustomResumeSource&& other) noexcept
      -> CustomResumeSource&
  {
    if (this != &other)
    {
      state = other.state;
      other.state = nullptr;
    }
    return *this;
  }

  void requestResume() const noexcept
  {
    auto expected = ropic::detail::ResumePhase::SUSPENDED;
    if (state->compare_exchange_strong(
            expected,
            ropic::detail::ResumePhase::READY,
            std::memory_order_seq_cst))
    {
      state->notify_one();
    }
  }
};

// =============================================================================
// Direct safe_awaiter fixtures using CustomResumeSource
// =============================================================================

/// Void result with CustomResumeSource, configurable resume mode.
struct CustomVoidResumeAwaiter
{
  ResumeMode mode;

  auto await_ready() -> bool { return false; }

  void await_suspend(CustomResumeSource rs) const
  {
    if (mode == ResumeMode::INLINE)
      rs.requestResume();
    else
      std::thread(
          [s = std::move(rs)]() mutable { s.requestResume(); })
          .detach();
  }

  void await_resume() {}
};

/// Typed result with CustomResumeSource, configurable resume mode.
template <typename T>
struct CustomValueResumeAwaiter
{
  T value;
  ResumeMode mode;

  auto await_ready() -> bool { return false; }

  void await_suspend(CustomResumeSource rs)
  {
    if (mode == ResumeMode::INLINE)
      rs.requestResume();
    else
      std::thread(
          [s = std::move(rs)]() mutable { s.requestResume(); })
          .detach();
  }

  auto await_resume() -> T { return std::move(value); }
};

/// Takes const CustomResumeSource&, inline resume.
template <typename T>
struct CustomConstRefResumeAwaiter
{
  T value;

  auto await_ready() -> bool { return false; }

  void await_suspend(const CustomResumeSource& rs)
  {
    rs.requestResume();
  }

  auto await_resume() -> T { return std::move(value); }
};

/// Takes CustomResumeSource&&, inline resume.
template <typename T>
struct CustomRvalueRefResumeAwaiter
{
  T value;

  auto await_ready() -> bool { return false; }

  void await_suspend(CustomResumeSource&& rs)
  {
    rs.requestResume();
  }

  auto await_resume() -> T { return std::move(value); }
};

// =============================================================================
// co_awaitable fixtures using CustomResumeSource
// =============================================================================

/// Member co_await returning CustomValueResumeAwaiter by value, inline.
struct CustomMemberCoAwtValue
{
  int value;
  auto operator co_await() -> CustomValueResumeAwaiter<int>
  {
    return {.value = value, .mode = ResumeMode::INLINE};
  }
};

/// Non-member co_await target using CustomResumeSource, inline.
struct CustomNonMemberValueTarget
{
  int value;
};

inline auto operator co_await(CustomNonMemberValueTarget t)
    -> CustomValueResumeAwaiter<int>
{
  return {.value = t.value, .mode = ResumeMode::INLINE};
}

// NOLINTEND(readability-identifier-naming,readability-convert-member-functions-to-static)
