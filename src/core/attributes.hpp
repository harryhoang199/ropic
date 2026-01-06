// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

/**
 * @file Attributes.hpp
 * @brief Cross-platform compiler attribute macros for optimization hints.
 *
 * Provides portable macros for force-inlining and coroutine heap allocation
 * elision (HALO) optimization hints across MSVC, GCC, and Clang.
 */

// ============================================================================
// ROPIC_FORCEINLINE - Cross-platform force inline attribute
// ============================================================================
// Forces the compiler to inline a function. Use on small, critical functions
// in the coroutine hot path to enable HALO optimization.

#if defined(_MSC_VER)
#  define ROPIC_FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#  define ROPIC_FORCEINLINE [[gnu::always_inline]] inline
#else
#  define ROPIC_FORCEINLINE inline
#endif

// ============================================================================
// ROPIC_CORO_AWAIT_ELIDABLE - Clang coroutine heap elision hint (Clang 18+)
// ============================================================================
// When applied to a coroutine return type, hints for aggressive HALO when:
// 1. Callee returns a type with this attribute
// 2. Callee is inlined
// 3. Return value is immediately co_awaited
//
// Empty on non-Clang compilers as this is a Clang extension.

#if defined(__clang__) && __has_cpp_attribute(clang::coro_await_elidable)
#  define ROPIC_CORO_AWAIT_ELIDABLE [[clang::coro_await_elidable]]
#  define ROPIC_HAS_CORO_ELISION_HINTS 1
#else
#  define ROPIC_CORO_AWAIT_ELIDABLE
#  define ROPIC_HAS_CORO_ELISION_HINTS 0
#endif
