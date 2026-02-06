// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <ropic.hpp>

#include "Error.hpp"

/// @brief Type alias for Either<VALUE, Error> for convenience.
template <typename VALUE>
using Result = ropic::Either<VALUE, Error>;
