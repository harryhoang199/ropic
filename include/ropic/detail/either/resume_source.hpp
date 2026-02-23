// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <atomic>
#include <cassert>

#include "ropic/detail/shared/resume_phase.hpp"

namespace ropic
{

/// @brief Copyable signal sender for resuming a suspended coroutine.
///
/// Given to external async operations (via SafeAwaitableAdapter) so they
/// can notify the coroutine chain when the operation completes.
/// Holds a non-owning pointer to the atomic state inside a ResumeHead.
///
/// @note This is a public API type: user-defined safe awaiters receive
/// a ResumeSource in their `await_suspend` and must call
/// `requestResume()` when the async result is available.
///
/// @see detail::ResumeHead — the shared data this class signals.
/// @see detail::ResumeTarget — the chain-side counterpart that consumes
///      the signal.
class ResumeSource
{
  std::atomic<detail::ResumePhase>* _state;

public:
  /// @brief Constructs a ResumeSource bound to a ResumeHead's state.
  /// @param state  Reference to the atomic phase variable in a ResumeHead.
  ResumeSource(std::atomic<detail::ResumePhase>& state)
      : _state(&state)
  {
    assert(
        _state && "ResumeSource: Cannot construct ResumeSource on null state");
  }

  /// @brief Copy constructor. Copies the state pointer (both instances
  /// can signal the same ResumeHead).
  ResumeSource(const ResumeSource&) = default;

  /// @brief Copy assignment. Copies the state pointer.
  auto operator=(const ResumeSource&) -> ResumeSource& = default;

  /// @brief Move constructor. Transfers ownership; source becomes null.
  ResumeSource(ResumeSource&& other) noexcept
      : _state(other._state)
  {
    other._state = nullptr;
  }

  /// @brief Move assignment. Transfers ownership; source becomes null.
  auto operator=(ResumeSource&& other) noexcept -> ResumeSource&
  {
    if (this != &other)
    {
      _state = other._state;
      other._state = nullptr;
    }
    return *this;
  }

  /// @brief Atomically signals that the async operation has completed.
  ///
  /// Transitions state from SUSPENDED to READY and wakes any thread
  /// blocked in ResumeTarget::waitAndResume(). If the state is no
  /// longer SUSPENDED (already READY or RESUMED), this is a no-op.
  ///
  /// @note Thread-safe. May be called from any thread. Must be called
  /// at most once per ResumeSource instance.
  void requestResume() const noexcept
  {
    assert(
        _state
        && "ResumeSource::requestResume: must not be called on null state");

    auto expected = detail::ResumePhase::SUSPENDED;
    if (_state->compare_exchange_strong(
            expected, detail::ResumePhase::READY, std::memory_order_seq_cst))
    {
      _state->notify_one();
    }
  }
};

} // namespace ropic
