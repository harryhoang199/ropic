// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "Error.hpp"
#include <ropic.hpp>

namespace ropic
{
  /// @brief Type alias for Either<DATA, Error> for convenience.
  template <typename DATA>
  using Result = Either<DATA, Error>;
}