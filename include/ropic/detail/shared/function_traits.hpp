// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <tuple>

namespace ropic
{
// NOLINTBEGIN(readability-identifier-naming)
// =========================================================================
// CORE BASE
// =========================================================================

/// @brief Helper struct to hold common type definitions.
///
/// Used to avoid code duplication across specializations.
///
/// @tparam RET The return type.
/// @tparam CLS The class type (void for free functions).
/// @tparam ARGS The argument types.
template <typename RET, typename CLS, typename... ARGS>
struct FunctionTraitsBase
{
  /// @brief The return type of the function.
  using ReturnType = RET;

  /// @brief The class that owns the function (void if free function).
  using ClassType = CLS;

  /// @brief A tuple containing all argument types.
  using ArgsTuple = std::tuple<ARGS...>;

  /// @brief The number of arguments.
  static constexpr std::size_t arity = sizeof...(ARGS);

  /// @brief Helper to get the type of the N-th argument.
  /// @tparam N The index of the argument.
  template <std::size_t N>
  using ArgType = std::tuple_element_t<N, ArgsTuple>;

  /// @brief The function signature type (e.g., int(float, double)). Useful for
  /// std::function<Signature>
  using Signature = RET(ARGS...);

  /// @brief The equivalent free function pointer type (e.g., int(*)(float,
  /// double)). For Lambdas, this is the type it WOULD decay to if stateless.
  using FreePointerType = RET (*)(ARGS...);
};

// =========================================================================
// PRIMARY TEMPLATE (HANDLES LAMBDAS & FUNCTORS)
// =========================================================================

/// @brief Primary template for FunctionTraits.
///
/// This handles Functors and Lambdas by inspecting their operator().
/// It inherits from FunctionTraits of the operator() member pointer.
///
/// @tparam T The type to analyze (Lambda, Functor, etc.).
template <typename T>
struct FunctionTraits : public FunctionTraits<decltype(&T::operator())>
{
};

// NOLINTBEGIN(bugprone-macro-parentheses)
// =========================================================================
// MACRO 1: FREE FUNCTIONS (Global/Static)
// =========================================================================

/// @brief Macro to generate specializations for Free Function Pointers.
/// @param IS_NOEXCEPT Boolean flag for noexcept.
/// @param NOEXCEPT_QUAL The noexcept specifier syntax.
#define DEFINE_FREE_TRAITS(IS_NOEXCEPT, NOEXCEPT_QUAL)                         \
  template <typename RET, typename... ARGS>                                    \
  struct FunctionTraits<RET (*)(ARGS...) NOEXCEPT_QUAL>                        \
      : public FunctionTraitsBase<RET, void, ARGS...>                          \
  {                                                                            \
                                                                               \
    static constexpr bool isConst = false;                                     \
    static constexpr bool isVolatile = false;                                  \
    static constexpr bool isLValueRef = false;                                 \
    static constexpr bool isRValueRef = false;                                 \
    static constexpr bool isNoexcept = IS_NOEXCEPT;                            \
    static constexpr bool isMember = false;                                    \
                                                                               \
    using PointerType = RET (*)(ARGS...) NOEXCEPT_QUAL;                        \
  };
// NOLINTEND(bugprone-macro-parentheses)

// Generate for Normal and Noexcept free functions
DEFINE_FREE_TRAITS(false, )
DEFINE_FREE_TRAITS(true, noexcept)

#undef DEFINE_FREE_TRAITS

// =========================================================================
// MACRO 2: MEMBER FUNCTIONS (24 Cases)
// =========================================================================

/// @brief Macro to generate specializations for Member Function Pointers.
#define DEFINE_MEMBER_TRAITS(                                                  \
    CV_QUAL,                                                                   \
    REF_QUAL,                                                                  \
    NOEXCEPT_QUAL,                                                             \
    IS_CONST,                                                                  \
    IS_VOLATILE,                                                               \
    IS_LREF,                                                                   \
    IS_RREF,                                                                   \
    IS_NOEXCEPT)                                                               \
  template <typename RET, typename CLS, typename... ARGS>                      \
  struct FunctionTraits<RET (CLS::*)(ARGS...) CV_QUAL REF_QUAL NOEXCEPT_QUAL>  \
      : public FunctionTraitsBase<RET, CLS, ARGS...>                           \
  {                                                                            \
                                                                               \
    static constexpr bool isConst = IS_CONST;                                  \
    static constexpr bool isVolatile = IS_VOLATILE;                            \
    static constexpr bool isLValueRef = IS_LREF;                               \
    static constexpr bool isRValueRef = IS_RREF;                               \
    static constexpr bool isNoexcept = IS_NOEXCEPT;                            \
    static constexpr bool isMember = true;                                     \
                                                                               \
    using PointerType = RET (CLS::*)(ARGS...) CV_QUAL REF_QUAL NOEXCEPT_QUAL;  \
  };

// --- Group 1: No Ref-Qualifier ---
DEFINE_MEMBER_TRAITS(, , , false, false, false, false, false)
DEFINE_MEMBER_TRAITS(, , noexcept, false, false, false, false, true)
DEFINE_MEMBER_TRAITS(const, , , true, false, false, false, false)
DEFINE_MEMBER_TRAITS(const, , noexcept, true, false, false, false, true)
DEFINE_MEMBER_TRAITS(volatile, , , false, true, false, false, false)
DEFINE_MEMBER_TRAITS(volatile, , noexcept, false, true, false, false, true)
DEFINE_MEMBER_TRAITS(const volatile, , , true, true, false, false, false)
DEFINE_MEMBER_TRAITS(const volatile, , noexcept, true, true, false, false, true)

// --- Group 2: L-Value Ref-Qualifier (&) ---
DEFINE_MEMBER_TRAITS(, &, , false, false, true, false, false)
DEFINE_MEMBER_TRAITS(, &, noexcept, false, false, true, false, true)
DEFINE_MEMBER_TRAITS(const, &, , true, false, true, false, false)
DEFINE_MEMBER_TRAITS(const, &, noexcept, true, false, true, false, true)
DEFINE_MEMBER_TRAITS(volatile, &, , false, true, true, false, false)
DEFINE_MEMBER_TRAITS(volatile, &, noexcept, false, true, true, false, true)
DEFINE_MEMBER_TRAITS(const volatile, &, , true, true, true, false, false)
DEFINE_MEMBER_TRAITS(const volatile, &, noexcept, true, true, true, false, true)

// --- Group 3: R-Value Ref-Qualifier (&&) ---
DEFINE_MEMBER_TRAITS(, &&, , false, false, false, true, false)
DEFINE_MEMBER_TRAITS(, &&, noexcept, false, false, false, true, true)
DEFINE_MEMBER_TRAITS(const, &&, , true, false, false, true, false)
DEFINE_MEMBER_TRAITS(const, &&, noexcept, true, false, false, true, true)
DEFINE_MEMBER_TRAITS(volatile, &&, , false, true, false, true, false)
DEFINE_MEMBER_TRAITS(volatile, &&, noexcept, false, true, false, true, true)
DEFINE_MEMBER_TRAITS(const volatile, &&, , true, true, false, true, false)
DEFINE_MEMBER_TRAITS(
    const volatile, &&, noexcept, true, true, false, true, true)

#undef DEFINE_MEMBER_TRAITS

// NOLINTEND(readability-identifier-naming)
} // namespace ropic
