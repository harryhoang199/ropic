// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <coroutine>
#include <type_traits>

#include "ropic/void.hpp"

#include "ropic/detail/shared/counting_gate.hpp"

#include "ropic/detail/either/either_promise.inl"

namespace ropic::detail
{

/**
 * @brief Awaiter for Either-to-Either composition with automatic error
 * propagation.
 *
 * Created by Promise::await_transform when co_await-ing an
 * EitherImpl<OTHER, DERIVED_ERR> inside an EitherImpl<VALUE, ERROR>
 * coroutine. Handles three outcomes:
 *
 * - **Data ready**: await_ready returns true; await_resume extracts value.
 * - **Suspended (async pending)**: links continuation chain and either
 *   performs symmetric transfer (READY) or propagates ResumeTarget to
 *   the tail node (SUSPENDED).
 * - **Error**: transfers error ownership to the tail promise and
 *   short-circuits the caller coroutine.
 *
 * @tparam VALUE          Success type of the caller coroutine.
 * @tparam ERROR          Error type of the caller coroutine.
 * @tparam OTHER          Success type of the awaited coroutine.
 * @tparam DERIVED_ERR    Error type of the awaited coroutine
 *                        (must be convertible to ERROR).
 * @tparam IS_EITHER_LREF True if the awaited EitherImpl is an lvalue
 *                        (await_resume yields lvalue ref).
 *
 * @see Promise::await_transform — creates this awaiter.
 * @see PromiseChainNode::findTail — used to locate the propagation target.
 */
template <
    typename VALUE,
    typename ERROR,
    typename OTHER,
    typename DERIVED_ERR,
    bool IS_EITHER_LREF>
class PropagatingAwaiter
{
  /// @brief Promise type of the caller coroutine (EitherImpl<VALUE, ERROR>).
  using CallerPromise = typename EitherImpl<VALUE, ERROR>::promise_type;

  /// @brief Promise type of the awaited coroutine
  /// (EitherImpl<OTHER, DERIVED_ERR>).
  using AwaitablePromise =
      typename EitherImpl<OTHER, DERIVED_ERR>::promise_type;

  /// @brief Reference to the awaited promise for accessing result/error.
  AwaitablePromise& _awaitablePromise;

public:
  /// @brief Constructs awaiter from the awaited coroutine's handle.
  /// @param awaitableHandle Handle to the awaited EitherImpl coroutine.
  explicit PropagatingAwaiter(
      std::coroutine_handle<AwaitablePromise> awaitableHandle)
      : _awaitablePromise{awaitableHandle.promise()}
  {
  }

  // NOLINTBEGIN(readability-identifier-naming)
  /// @brief Returns true if the awaited coroutine already has data.
  /// @return `true` if data is available (no suspend needed), `false`
  ///         if suspended or errored.
  [[nodiscard]]
  auto await_ready() noexcept -> bool
  {
    return _awaitablePromise.result.has_value();
  }

  /// @brief Handles suspension: links the continuation chain, propagates
  /// error or ResumeTarget, and determines the next coroutine to resume.
  ///
  /// @param callerHandle  Handle to the caller coroutine.
  /// @return A coroutine handle for symmetric transfer:
  ///         - The resume handle if the async op is READY.
  ///         - `std::noop_coroutine()` otherwise (caller stays suspended).
  ///
  /// @note Always noexcept: only assigns raw pointers / moves ResumeTarget.
  [[nodiscard]]
  auto await_suspend(std::coroutine_handle<CallerPromise> callerHandle) noexcept
      -> std::coroutine_handle<>
  {
    assert(
        (_awaitablePromise.continuation == nullptr)
        && "PropagatingAwaiter::await_suspend: awaitable must not have an "
           "existing continuation (indicates coroutine composition bug)");

    auto callerNode = PromiseChainNode::toBaseHandle(callerHandle);

    if (_awaitablePromise.error != nullptr)
    {
      auto tailNode = PromiseChainNode::findTail(callerNode);
      // Transfer error ownership to the tail promise
      tailNode.promise().error = _awaitablePromise.error;
      // Reset _awaitablePromise error to avoid destruction
      _awaitablePromise.error = nullptr;
      return std::noop_coroutine();
    }

    // Build the continuation chain
    _awaitablePromise.continuation = callerNode;

    auto& resumeTarget = _awaitablePromise.resumeTarget;
    if (!resumeTarget)
      return std::noop_coroutine();

    // Attempt to resume as soon as resumeTarget's state becomes READY

#ifdef ROPIC_TESTING_MODE
    s_awaitSuspendGate.passThrough();
#endif

    assert(
        (resumeTarget.state() != ResumePhase::RESUMED)
        && "PropagatingAwaiter::await_suspend: ResumeTarget must not be "
           "already consumed (state should be SUSPENDED or READY)");

    auto resumeHandle = resumeTarget.tryClaimHandle();
    if (!resumeHandle)
    {
      auto tailNode = PromiseChainNode::findTail(callerNode);
      // Still SUSPENDED: transfer ResumeTarget to the tail node
      tailNode.promise().resumeTarget = std::move(resumeTarget);
      return std::noop_coroutine();
    }

    // READY -> RESUMED: symmetric transfer
    return resumeHandle;
  }

  /// @brief Extracts the data value from the awaited promise.
  ///
  /// @return `OTHER&` if the awaited EitherImpl was an lvalue,
  ///         `OTHER&&` (moved) if rvalue, or `void` if OTHER is Void.
  ///
  /// @pre The awaited promise must contain data (not error or empty).
  [[nodiscard]]
  auto await_resume() noexcept(noexcept(
      !std::is_constructible_v<OTHER, OTHER&&>
      || std::is_nothrow_move_constructible_v<OTHER>)) -> decltype(auto)
  {
    if constexpr (std::same_as<OTHER, Void>)
      return;
    else
    {
      auto& r = _awaitablePromise.result;
      assert(
          r.has_value()
          && "PropagatingAwaiter::await_resume: awaited EitherImpl must "
             "contain data (error case should not reach here)");

      if constexpr (IS_EITHER_LREF)
        return (r.value()); // NOTE: () is used as a reference operator
      else if constexpr (std::is_constructible_v<OTHER, OTHER&&>)
        return std::move(r.value());
    }
  }

#ifdef ROPIC_TESTING_MODE
  /// @brief Test-only counting gate that controls execution flow
  /// within await_suspend for deterministic testing of resumeTarget
  /// propagation.
  ///
  /// @see CountingGate for full semantics (negative = open without
  ///      consuming permits, zero = closed until waitAndReopen(),
  ///      positive N = N free passes with atomic decrement).
  CountingGate inline static s_awaitSuspendGate;
#endif
  // NOLINTEND(readability-identifier-naming)
};
} // namespace ropic::detail
