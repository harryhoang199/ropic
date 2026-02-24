// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <concepts>

#include "ropic/detail/either/resume_target.hpp"

#pragma once

namespace ropic::detail
{
/**
 * @brief Non-template base for coroutine continuation chains and type-erased
 * error storage.
 *
 * Enables heterogeneous error-type chains (e.g., Promise<BaseError> →
 * Promise<NetworkError>) by storing both continuation and error as
 * type-erased members. All chain-walking and error-transfer code operates
 * through this single type, avoiding coroutine_handle type mismatches.
 *
 * @see EitherImpl::Promise — the concrete derived type.
 * @see ResumeHead — the opposite end of the chain (innermost / head).
 */
struct PromiseChainNode
{
  /// @brief Handle to the caller coroutine (next node toward the tail).
  /// Null for the outermost currently-linked node.
  std::coroutine_handle<PromiseChainNode> continuation = nullptr;

  /// @brief Heap-allocated error (type-erased), or nullptr if no error.
  ///
  /// @note Owned by the concrete Promise that inherits from this node.
  /// The derived destructor must `delete static_cast<ERROR*>(error)`.
  void* error = nullptr;

  /// @brief Resume target propagated from the innermost suspended coroutine.
  ///
  /// @see ResumeTarget for ownership and atomic state details.
  ResumeTarget resumeTarget;

  /// @brief Upcasts a typed coroutine handle to the base
  /// PromiseChainNode handle.
  ///
  /// Enables type-erased chain operations on any Promise derived from
  /// PromiseChainNode.
  ///
  /// @tparam PROMISE  Concrete promise type derived from PromiseChainNode.
  /// @param h  Typed coroutine handle to upcast.
  /// @return Type-erased handle addressing the same coroutine frame.
  template <typename PROMISE>
    requires std::derived_from<PROMISE, PromiseChainNode>
  [[nodiscard]]
  auto static toBaseHandle(std::coroutine_handle<PROMISE> h) noexcept
      -> std::coroutine_handle<PromiseChainNode>
  {
    return std::coroutine_handle<PromiseChainNode>::from_promise(h.promise());
  }

  /// @brief Walks the continuation chain and returns the last node
  /// (where continuation is null).
  ///
  /// The tail is the outermost currently-linked node in the chain.
  ///
  /// @note The tail is not necessarily the absolute root coroutine.
  /// The chain is built incrementally; symmetric transfer in
  /// PropagatingAwaiter::await_suspend can stop propagation early,
  /// leaving outer coroutines unlinked.
  ///
  /// @param fromHandle  Starting node to walk from.
  /// @return The last node in the chain reachable from @p fromHandle.
  ///
  /// @see ResumeHead — represents the opposite end (head / innermost).
  [[nodiscard]]
  auto static findTail(
      std::coroutine_handle<PromiseChainNode> fromHandle) noexcept
      -> std::coroutine_handle<PromiseChainNode>
  {
    // Walk to the tail of the continuation chain
    while (fromHandle.promise().continuation)
    {
      fromHandle = fromHandle.promise().continuation;
    }
    return fromHandle;
  }
};

} // namespace ropic::detail
