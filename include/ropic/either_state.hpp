// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cstdint>

namespace ropic
{

/// @brief Overall state of an Either coroutine, as observed by the caller.
///
/// Returned by `Either::state()` to indicate what actions are available.
///
/// @see Either::state(), Either::resume()
enum class CoroState : std::uint8_t
{
  UNDEFINED, ///< Not observable: moved-from, or suspended via
             ///< a non-trackable (standard) awaiter.
  PENDING,   ///< Trackable suspension: async operation in progress.
             ///< Can call resume() (blocks until ready).
  READY,     ///< Trackable suspension: async operation complete.
             ///< Can call resume() (returns immediately).
  DONE,      ///< Coroutine completed. value() or error() is available.
};

} // namespace ropic
