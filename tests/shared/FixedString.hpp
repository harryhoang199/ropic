// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cstddef>

// NOLINTBEGIN(modernize-avoid-c-arrays)

/// @brief Compile-time fixed-length string that satisfies the C++20
/// "structural type" requirement for use as a non-type template parameter
/// (NTTP). This enables string-literal template arguments such as:
///
///   template<FixedString ID> struct Tracker { ... };
///   Tracker<"my-id"> t;   // compiler deduces FixedString<6>
///
/// @tparam N  The length of the stored string (excluding null terminator).
///            Automatically deduced from string literals via the deduction
///            guide below.
///
/// **Structural type requirements (C++20 [temp.param]/7):**
///   - All base classes and non-static data members are public.
///   - All non-static data members are structural types or arrays thereof.
///   - The class has no user-declared copy/move constructors or destructors.
///
/// Because the only data member is `char buffer[N + 1]` (a public array of
/// a scalar type) and no special members are declared, `FixedString`
/// satisfies all three requirements.
template <size_t N>
struct FixedString
{
  char buffer[N + 1]{};

  /// @brief Constructs from a reference to a fixed-size character array.
  /// Copies exactly N characters into the internal buffer and ensures
  /// null termination. This is the sole constructor, which eliminates
  /// the overload ambiguity that would arise from having both a
  /// `const char*` and a `const char (&)[N]` constructor.
  constexpr FixedString(char const (&s)[N])
  {
    for (size_t i = 0; i != N; ++i)
      buffer[i] = s[i];

    buffer[N] = 0;
  }
};

/// @brief Deduction guide: lets the compiler deduce N from a string literal.
template <size_t N>
FixedString(char const (&)[N]) -> FixedString<N>;

// NOLINTEND(modernize-avoid-c-arrays)
