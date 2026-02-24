// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <atomic>
#include <coroutine>
#include <type_traits>

#include "ropic/safe_awaiter_traits.hpp"

#include "ropic/detail/either/promise_chain_node.hpp"
#include "ropic/detail/either/resume_head.hpp"

namespace ropic::detail
{

template <typename VALUE, typename ERROR>
class EitherImpl;

/// @brief Adapts a `safe_awaitable` to the standard C++ awaiter interface,
/// integrating it with the Either resume mechanism.
///
/// Wraps a user-provided safe awaiter and, on suspension:
/// 1. Stores the caller handle in a local ResumeHead.
/// 2. Propagates the ResumeTarget to the tail of the continuation chain.
/// 3. Passes a ResumeSource (bound to the ResumeHead) to the inner
///    awaiter's `await_suspend`, so the async operation can signal
///    completion.
///
/// @tparam VALUE     Success type of the enclosing Either coroutine.
/// @tparam ERROR     Error type of the enclosing Either coroutine.
/// @tparam SAFE_AWT  The safe_awaitable type being adapted.
///
/// @see safe_awaitable — the concept this adapter requires.
/// @see ResumeHead — the shared data owned by this adapter.
/// @see Promise::await_transform — creates this adapter.
template <typename VALUE, typename ERROR, typename SAFE_AWT>
  requires(safe_awaitable<SAFE_AWT>)
class SafeAwaitableAdapter
{
  /// @brief The concrete awaiter type extracted from SAFE_AWT.
  using SafeAwaiter = typename SafeAwaiterTraits<SAFE_AWT>::Awaiter;

  /// @brief The ResumeSource type accepted by the inner awaiter's
  /// `await_suspend`.
  using ResumeSource =
      std::decay_t<typename SafeAwaiterTraits<SAFE_AWT>::SuspendArgType>;

  /// @brief The wrapped awaiter delegated to for ready/suspend/resume.
  SafeAwaiter _safeAwaiter;

  /// @brief Local resume data; lifetime is tied to this adapter
  /// (which lives on the coroutine frame).
  ResumeHead _resumeHead;

  /// @brief Typed handle for the enclosing Either coroutine.
  using EitherHandle =
      std::coroutine_handle<typename EitherImpl<VALUE, ERROR>::promise_type>;

public:
  /// @brief Constructs the adapter by initializing the inner awaiter.
  /// @param safeAwaitable  The safe_awaitable to adapt.
  explicit SafeAwaitableAdapter(SAFE_AWT&& safeAwaitable)
      : _safeAwaiter(initAwaiter(std::forward<SAFE_AWT>(safeAwaitable)))
  {
  }

  // NOLINTBEGIN(readability-identifier-naming)

  /// @brief Delegates to the inner awaiter's readiness check.
  /// @return `true` if the async result is already available.
  [[nodiscard]]
  auto await_ready() noexcept(noexcept(_safeAwaiter.await_ready()))
      -> decltype(auto)
  {
    return _safeAwaiter.await_ready();
  }

  /// @brief Sets up the resume mechanism and delegates suspension to
  /// the inner awaiter.
  ///
  /// Stores the caller handle in the local ResumeHead, propagates
  /// the ResumeTarget to the tail node, then forwards a ResumeSource
  /// to the inner awaiter so it can signal completion.
  ///
  /// @param callerHandle  Handle to the enclosing Either coroutine.
  /// @return Whatever the inner awaiter's `await_suspend` returns.
  [[nodiscard]]
  auto await_suspend(EitherHandle callerHandle) noexcept(
      noexcept(_safeAwaiter.await_suspend(std::declval<ResumeSource>())))
      -> decltype(auto)
    requires std::constructible_from<ResumeSource, std::atomic<ResumePhase>&>
  {
    _resumeHead.handle = callerHandle;

    auto callerNode = PromiseChainNode::toBaseHandle(callerHandle);
    auto tailNode = PromiseChainNode::findTail(callerNode);

    tailNode.promise().resumeTarget.setResumeHead(_resumeHead);

    return _safeAwaiter.await_suspend(ResumeSource{_resumeHead.state});
  }

  /// @brief Delegates to the inner awaiter's resume logic.
  /// @return Whatever the inner awaiter's `await_resume` returns.
  [[nodiscard]]
  auto await_resume() noexcept(noexcept(_safeAwaiter.await_resume()))
      -> decltype(auto)
  {
    return _safeAwaiter.await_resume();
  }

private:
  /// @brief Initializes the inner awaiter from the given awaitable.
  ///
  /// If the awaitable is already a safe_awaiter, forwards it directly.
  /// Otherwise, invokes `operator co_await` to obtain the awaiter.
  ///
  /// @param awaitable  The safe_awaitable to extract an awaiter from.
  /// @return The initialized awaiter.
  [[nodiscard]]
  auto static initAwaiter(SAFE_AWT&& awaitable) -> decltype(auto)
  {
    if constexpr (safe_awaiter<SAFE_AWT>)
      return std::forward<SAFE_AWT>(awaitable);
    else
      return invokeCoAwait(std::forward<SAFE_AWT>(awaitable));
  }
  // NOLINTEND(readability-identifier-naming)
};

} // namespace ropic::detail
