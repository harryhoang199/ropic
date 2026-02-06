// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <type_traits>
#include <variant>

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
 * - Not std::monostate (reserved for internal empty state)
 *
 * @code
 * static_assert(plain_value_type<int>);           // OK
 * static_assert(plain_value_type<std::string>);   // OK
 * static_assert(!plain_value_type<int&>);         // Fail: reference
 * static_assert(!plain_value_type<const int>);    // Fail: const-qualified
 * static_assert(!plain_value_type<void>);         // Fail: void
 * @endcode
 */
template <typename T>
concept plain_value_type = std::is_object_v<T>
                        && std::is_destructible_v<T>
                        && std::same_as<T, std::remove_cvref_t<T>>
                        && !std::same_as<T, std::monostate>
                        && !std::is_void_v<T>;

/**
 * @brief Concept validating VALUE and ERROR types for EitherImpl.
 *
 * Both types must satisfy plain_value_type and must be distinct from each
 * other. This prevents ambiguity when constructing or accessing the Either.
 *
 * @code
 * static_assert(either_concept<int, std::string>);     // OK
 * static_assert(!either_concept<int, int>);            // Fail: same type
 * static_assert(!either_concept<int&, std::string>);   // Fail: VALUE is ref
 * @endcode
 */
template <typename VALUE, typename ERROR>
concept either_concept = plain_value_type<VALUE>
                      && plain_value_type<ERROR>
                      && !std::is_same_v<VALUE, ERROR>;

} // namespace ropic::detail
