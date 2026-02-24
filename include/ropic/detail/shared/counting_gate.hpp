// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <atomic>
#include <cassert>
#include <utility>

namespace ropic::detail
{

/// @brief Test-only counting gate for deterministic control of execution
/// flow in await_suspend.
///
/// @details Acts as a counting gate with four states:
/// - **OPEN** (default, -1): gate is permanently open;
///   passThrough() proceeds without consuming a permit,
///   waitAndReopen() invokes its callback without modifying
///   the counter.
/// - **CLOSED** (0): gate is closed; passThrough() spin-waits
///   until an external thread calls waitAndReopen().
/// - **CLAIMED** (-2): transient state used by waitAndReopen()
///   to hold the gate closed while executing its callback;
///   passThrough() spin-waits on this state just like CLOSED.
/// - **Positive N**: N free passes; each passThrough() atomically
///   consumes one permit. After N passes the counter reaches
///   CLOSED and the next call blocks.
///
/// State transitions:
/// @code
///   reset(N>0)       passThrough()        waitAndReopen()
///   --------->  N  ----------------> 0  -----------------> -2
///                                   (CLOSED)             (CLAIMED)
///                                                           |
///                                              fn() done    |
///                                                           v
///   reset(OPEN)                                            -1
///   --------->  -1 (OPEN)  <-------------------------------'
/// @endcode
///
/// This enables test scenarios that control whether
/// `ResumeSource::requestResume()` fires before or after
/// `ResumeTarget::tryClaimHandle()` in nested coroutine chains.
///
/// @see PropagatingAwaiter::s_awaitSuspendGate
class CountingGate
{
  enum State : int // NOLINT(performance-enum-size)
  {
    CLAIMED = -2, ///< Gate held by waitAndReopen(); fn() in progress.
    OPEN = -1,    ///< Gate permanently open; passThrough() passes freely.
    CLOSED = 0,   ///< Gate closed; passThrough() spin-waits.
  };

  std::atomic_int _counter;

public:
  /// @brief Constructs gate with the given initial counter value.
  /// @param initialPermits Counter value (default OPEN = permanently open).
  explicit CountingGate(int initialPermits = State::OPEN) noexcept
      : _counter{initialPermits}
  {
  }

  /// @brief Resets the counter to the given value.
  ///
  /// @param permits New counter value. Must be greater than CLAIMED
  ///        (i.e., >= OPEN). Use OPEN (-1) for permanently open,
  ///        CLOSED (0) for closed, or a positive N for N free passes.
  ///
  /// @pre Must not be called while passThrough() or waitAndReopen()
  ///      is active on another thread.
  void reset(int permits) noexcept
  {
    assert(
        (permits > State::CLAIMED)
        && "CountingGate::reset: permits must not overlap internal "
           "sentinel values (CLAIMED and below are reserved)");
    _counter.store(permits, std::memory_order_seq_cst);
  }

  /// @brief Spin-waits while gate is CLOSED or CLAIMED, then consumes
  /// one permit if positive, or passes freely if OPEN.
  ///
  /// Behavior depends on the current counter state:
  ///
  /// - **CLOSED / CLAIMED**: spin-waits until waitAndReopen() reopens
  ///   the gate to OPEN.
  ///
  /// - **OPEN**: returns immediately without modifying the counter.
  ///
  /// - **Positive N**: atomically decrements the counter (consuming
  ///   one permit) and returns.
  ///
  /// Used inside PropagatingAwaiter::await_suspend to gate execution.
  void passThrough() noexcept
  {
    while (true)
    {
      int val = _counter.load(std::memory_order_seq_cst);
      if (val == State::CLOSED || val == State::CLAIMED)
        continue;

      if (val == State::OPEN)
        return;

      assert(
          (val > 0)
          && "CountingGate::passThrough: counter must not be negative "
             "(only OPEN and CLAIMED are valid negative states)");

      if (_counter.compare_exchange_weak(
              val, val - 1, std::memory_order_seq_cst))
        return;
    }
  }

  /// @brief Spin-waits until counter reaches CLOSED or OPEN, invokes
  /// @p fn, then reopens the gate.
  ///
  /// Behavior depends on the current counter state:
  ///
  /// - **OPEN**: invokes @p fn immediately without modifying the
  ///   counter. Multiple callers may invoke @p fn concurrently;
  ///   @p fn itself must be thread-safe in this case.
  ///
  /// - **CLOSED**: atomically transitions to CLAIMED, invokes @p fn,
  ///   then stores OPEN. Only one caller wins the CAS; others
  ///   spin on CLAIMED until the gate reopens. This guarantees
  ///   exclusive execution and that @p fn completes before any
  ///   passThrough() caller is released.
  ///
  /// - **Positive N**: spin-waits until permits are consumed down
  ///   to CLOSED.
  ///
  /// - **CLAIMED**: spin-waits until the current holder reopens
  ///   the gate (supports multiple waitAndReopen() callers).
  ///
  /// @tparam FUNC Callable with signature compatible with `void()`.
  /// @param fn  Action to execute while the gate is closed.
  template <typename FUNC>
  void waitAndReopen(FUNC&& fn) noexcept(noexcept(std::forward<FUNC>(fn)()))
  {
    while (true)
    {
      int val = _counter.load(std::memory_order_seq_cst);
      if (val > 0 || val == State::CLAIMED)
        continue;

      if (val == State::OPEN)
      {
        std::forward<FUNC>(fn)();
        return;
      }

      if (_counter.compare_exchange_weak(
              val, State::CLAIMED, std::memory_order_seq_cst))
      {
        std::forward<FUNC>(fn)();
        _counter.store(State::OPEN, std::memory_order_seq_cst);
        return;
      }
    }
  }

  /// @brief Returns the current counter value.
  ///
  /// Useful for test assertions to verify how many passes occurred.
  /// After waitAndReopen() completes, count() returns OPEN (-1).
  [[nodiscard]]
  auto count() const noexcept -> int
  {
    return _counter.load(std::memory_order_seq_cst);
  }
};

} // namespace ropic::detail
