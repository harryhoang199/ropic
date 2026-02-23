// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cstdint>

namespace ropic::detail
{

/// @brief Internal state of a ResumeHead's atomic variable.
///
/// Describes the phase of the resume mechanism lifecycle:
/// SUSPENDED -> READY -> RESUMED (always progresses forward, never reverses).
///
/// @see ResumeHead, ResumeSource, ResumeTarget
enum class ResumePhase : std::uint8_t
{
  SUSPENDED, ///< Waiting for external signal via ResumeSource.
  READY,     ///< Signal received; ready for resume.
  RESUMED,   ///< Handle has been claimed or executed.
};

} // namespace ropic::detail
