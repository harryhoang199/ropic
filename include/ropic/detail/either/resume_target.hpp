// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <coroutine>

#include "resume_head.hpp"

#include "ropic/detail/shared/resume_phase.hpp"

namespace ropic::detail
{

/// @brief Move-only owning accessor to a ResumeHead, providing atomic
/// state queries and resume control.
///
/// Used internally by the promise chain to track a suspended async
/// operation and either:
/// - claim the coroutine handle for symmetric transfer, or
/// - block until ready and resume the coroutine directly.
///
/// @note Move semantics transfer the pointer and null-out the source,
/// preventing double-claim. Each ResumeTarget instance represents
/// exclusive access to its ResumeHead.
///
/// @see ResumeHead — the shared data this class accesses.
/// @see ResumeSource — the external signal sender (counterpart).
class ResumeTarget
{
  ResumeHead* _resumeHead = nullptr;

public:
  // Move-only: prevents accidental double-claim of the same ResumeHead.
  ResumeTarget() = default;

  ResumeTarget(const ResumeTarget&) = delete;
  auto operator=(const ResumeTarget&) -> ResumeTarget& = delete;

  /// @brief Move constructor. Transfers ownership; source becomes null.
  ResumeTarget(ResumeTarget&& other) noexcept
      : _resumeHead(other._resumeHead)
  {
    other._resumeHead = nullptr;
  }

  /// @brief Move assignment. Transfers ownership; source becomes null.
  auto operator=(ResumeTarget&& other) noexcept -> ResumeTarget&
  {
    if (this != &other)
    {
      _resumeHead = other._resumeHead;
      other._resumeHead = nullptr;
    }
    return *this;
  }

  /// @brief Atomically tries to claim the resume handle.
  ///
  /// If state is READY, transitions to RESUMED and returns the handle
  /// for symmetric transfer. If state is SUSPENDED, returns nullptr
  /// (the async operation has not completed yet).
  ///
  /// @pre `*this` must be non-null. State must be SUSPENDED or READY.
  /// @post On success (READY → RESUMED): `*this` becomes null.
  ///       On failure (still SUSPENDED): `*this` remains valid.
  ///
  /// @return The coroutine handle on success, or nullptr if still
  ///         SUSPENDED.
  ///
  /// @note Must be called at most once per ResumeTarget instance
  /// because `_resumeHead` is consumed on success.
  [[nodiscard]]
  auto tryClaimHandle() noexcept -> std::coroutine_handle<>
  {
    assert(
        _resumeHead
        && "ResumeTarget::tryClaimHandle: must not be called on null target "
           "(check operator bool() first)");

    auto expected = ResumePhase::READY;
    if (_resumeHead->state.compare_exchange_strong(
            expected, ResumePhase::RESUMED, std::memory_order_seq_cst))
    {
      auto handle = _resumeHead->handle;
      _resumeHead = nullptr;

      return handle;
    }

    assert(
        (expected == ResumePhase::SUSPENDED)
        && "ResumeTarget::tryClaimHandle: phase must be SUSPENDED or READY "
           "(RESUMED is invalid)");

    return nullptr;
  }

  /// @brief Returns the current phase of the resume mechanism.
  ///
  /// @pre `*this` must be non-null.
  /// @return The current ResumePhase (SUSPENDED, READY, or RESUMED).
  [[nodiscard]]
  auto state() const noexcept -> ResumePhase
  {
    assert(
        _resumeHead
        && "ResumeTarget::state: must not be called on null target "
           "(check operator bool() first)");
    return _resumeHead->state.load(std::memory_order_seq_cst);
  }

  /// @brief Blocks until the phase becomes READY, transitions to RESUMED,
  /// then resumes the coroutine.
  ///
  /// Uses atomic wait/notify for efficient blocking. If the state is
  /// already READY, transitions immediately without waiting.
  ///
  /// @pre `*this` must be non-null. State must be SUSPENDED or READY.
  /// @post `*this` becomes null. The coroutine has been resumed.
  ///
  /// @note Must be called at most once per ResumeTarget instance
  /// because `_resumeHead` is consumed after resumption.
  void waitAndResume() noexcept
  {
    assert(
        _resumeHead
        && "ResumeTarget::waitAndResume: must not be called on null target "
           "(check operator bool() first)");

    auto expected = ResumePhase::READY;
    while (!_resumeHead->state.compare_exchange_strong(
        expected, ResumePhase::RESUMED, std::memory_order_seq_cst))
    {
      if (expected == ResumePhase::RESUMED)
        return;

      _resumeHead->state.wait(
          ResumePhase::SUSPENDED, std::memory_order_seq_cst);
      expected = ResumePhase::READY;
    }

    auto handle = _resumeHead->handle;
    _resumeHead = nullptr;
    assert(
        handle
        && "ResumeTarget::waitAndResume: resumeHead handle must not be null");

    handle.resume();
  }

  /// @brief Binds this target to a ResumeHead.
  /// @param resumeHead  The ResumeHead to point to.
  void setResumeHead(ResumeHead& resumeHead) noexcept
  {
    _resumeHead = &resumeHead;
  }

  /// @brief Returns true if this target refers to a ResumeHead.
  /// @return `true` if non-null, `false` otherwise.
  [[nodiscard]]
  explicit operator bool() const noexcept
  {
    return _resumeHead != nullptr;
  }
};

} // namespace ropic::detail
