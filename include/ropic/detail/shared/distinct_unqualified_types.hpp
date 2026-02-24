// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "ropic/detail/shared/unqualified_type.hpp"

namespace ropic::detail
{
/**
 * @brief Concept validating VALUE and ERROR types for EitherImpl.
 *
 * Both types must satisfy unqualified_type and must be distinct from
 * each other. This prevents ambiguity when constructing or accessing the
 * Either.
 */
template <typename T1, typename T2>
concept distinct_unqualified_types =
    unqualified_type<T1> && unqualified_type<T2> && !std::same_as<T1, T2>;

} // namespace ropic::detail
