// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <ropic.hpp>

#include "Error.hpp"

/// @brief Type alias for Either<DATA, Error> for convenience.
template <typename DATA>
using Result = ropic::Either<DATA, Error>;
