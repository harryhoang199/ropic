// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <variant>

namespace ropic::detail
{
/**
 * @brief Concept ensuring a type is suitable for Either storage.
 *
 * A plain value type must satisfy all of the following:
 * - Not a reference type
 * - Not const or volatile qualified
 * - Not void
 * - Not std::monostate (reserved for internal empty state)
 */
template <typename T>
concept plain_value_type = std::same_as<T, std::remove_cvref_t<T>>
                        && !std::same_as<T, std::monostate>
                        && !std::is_void_v<T>;

/**
 * @brief Concept validating DATA and ERROR types for EitherImpl.
 *
 * Both types must satisfy plain_value_type and must be distinct from each
 * other. This prevents ambiguity when constructing or accessing the Either.
 */
template <typename DATA, typename ERROR>
concept either_concept = plain_value_type<DATA>
                      && plain_value_type<ERROR>
                      && !std::is_same_v<DATA, ERROR>;

} // namespace ropic::detail
