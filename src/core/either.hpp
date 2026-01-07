// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "either_impl.hpp"

namespace ropic
{
/**
 * @brief Railway-oriented result type: holds either data or error.
 *
 * A template alias that provides a convenient interface for error handling
 * using the Railway Oriented Programming pattern. Supports both direct value
 * construction and C++20 coroutines with automatic error propagation.
 *
 * When DATA is `void`, the type automatically substitutes `Void` to maintain
 * variant compatibility. Use `Either<Void, ERROR>` or `Either<void, ERROR>`
 * for operations that succeed without returning a value.
 *
 * @tparam DATA The success value type. If void, substituted with Void.
 * @tparam ERROR The error type. Must differ from DATA.
 *
 * @see detail::EitherImpl for implementation details.
 * @see Void for void-like success value.
 *
 * @code
 * // Coroutine with automatic error propagation
 * Either<int, Error> compute(int x) {
 *     if (x < 0) co_return Error{ErrorTag::VALIDATION, "Negative"};
 *     co_return x * 2;
 * }
 *
 * // Composing coroutines - errors propagate automatically via co_await
 * Either<int, Error> composed(int x) {
 *     auto value = co_await compute(x);
 *     co_return value + 10;
 * }
 *
 * // Checking results
 * auto result = compute(5);
 * if (auto err = result.error()) {
 *     std::cout << err->message();
 * } else {
 *     std::cout << *result.data();
 * }
 * @endcode
 */
template <typename DATA, typename ERROR>
using Either = std::conditional_t<
    std::is_same_v<DATA, void>,
    detail::EitherImpl<Void, ERROR>,
    detail::EitherImpl<DATA, ERROR>>;
} // namespace ropic