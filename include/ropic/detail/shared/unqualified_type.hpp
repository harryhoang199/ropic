// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <concepts>

namespace ropic::detail
{
/**
 * @brief Concept ensuring a type is suitable for Either storage.
 *
 * A plain value type must satisfy all of the following:
 * - Is an object type (not function, reference, or void)
 * - Destructible (has accessible non-deleted destructor)
 * - Not a reference type
 * - Not const or volatile qualified
 * - Not void
 */
template <typename T>
concept unqualified_type = std::is_object_v<T>
                        && std::same_as<T, std::remove_cvref_t<T>>
                        && std::is_destructible_v<T>;
} // namespace ropic::detail
