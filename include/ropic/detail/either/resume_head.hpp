// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <atomic>
#include <coroutine>

#include "ropic/detail/shared/resume_phase.hpp"

namespace ropic::detail
{

/// @brief Shared resumption data for the innermost suspended coroutine
/// in a continuation chain.
///
/// Created by SafeAwaitableAdapter on the coroutine frame. Accessed
/// externally via ResumeSource (signal sender) and internally via
/// ResumeTarget (signal receiver on the chain).
///
/// @note Lifetime is tied to the SafeAwaitableAdapter that owns it.
/// Both ResumeSource and ResumeTarget hold raw pointers to this object;
/// the adapter must outlive them.
///
/// @see ResumeSource — non-owning writer; signals readiness via `state`.
/// @see ResumeTarget — owning reader; claims `handle` for resumption.
/// @see PromiseChainNode::resumeTarget — chain-side accessor to this head.
/// @see PromiseChainNode::findTail — locates the tail (opposite end).
struct ResumeHead
{
  /// @brief Coroutine handle to resume when the async operation completes.
  ///
  /// Points to the innermost coroutine that directly suspended on
  /// the async operation. Resumption cascades outward via symmetric
  /// transfer through the continuation chain.
  std::coroutine_handle<> handle = nullptr;

  /// @brief Atomic phase coordinating ResumeSource and ResumeTarget.
  ///
  /// Transitions: SUSPENDED → READY (via ResumeSource::requestResume())
  ///            → RESUMED (via ResumeTarget::tryClaimHandle()).
  /// Monotonic: never reverses direction.
  std::atomic<ResumePhase> state{ResumePhase::SUSPENDED};
};

} // namespace ropic::detail
