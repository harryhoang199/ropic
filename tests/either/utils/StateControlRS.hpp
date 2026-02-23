// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <atomic>

#include "ropic/detail/shared/resume_phase.hpp"

/// @brief Custom resume source exposing the raw atomic state reference.
/// Used by test awaiters (e.g., HoldStateAwaiter, GatedCorruptAwaiter) to
/// give tests direct control over ResumeHead state for timing and
/// corruption scenarios.
///
/// Satisfies the resume_source concept required by SafeAwaitableAdapter
/// (constructible from std::atomic<ResumePhase>&, has requestResume()).
struct StateControlRS
{
  std::atomic<ropic::detail::ResumePhase>& state;

  StateControlRS(std::atomic<ropic::detail::ResumePhase>& s)
      : state(s)
  {
  }

  void requestResume() const noexcept
  {
    auto expected = ropic::detail::ResumePhase::SUSPENDED;
    if (state.compare_exchange_strong(
            expected,
            ropic::detail::ResumePhase::READY,
            std::memory_order_seq_cst))
    {
      state.notify_one();
    }
  }
};
