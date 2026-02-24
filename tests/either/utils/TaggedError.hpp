// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <string>
#include <utility>

#include "../../shared/FixedString.hpp"

/// @brief Per-test error type parameterized on a compile-time string ID.
/// Each unique ID produces a distinct type, ensuring separate
/// PropagatingAwaiter specializations (and thus separate
/// CountingGate s_awaitSuspendGate instances) per test case.
template <FixedString ID>
struct TaggedError
{
  std::string message;

  TaggedError(std::string msg)
      : message(std::move(msg))
  {
  }
};
