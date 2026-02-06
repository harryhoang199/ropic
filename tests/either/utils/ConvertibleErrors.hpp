// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <string>
#include <utility>

struct NetworkError; // Forward declaration; defined in TestHelpers.hpp

/// @brief Independent error hierarchy with conversion operators
/// (NOT derived from BaseError). Used for "convertible but not derived"
/// cross-type co_await tests.

/// @brief Top-level error in the convertible chain. Simplest form.
struct LevelAError
{
  int code;
  std::string message;

  LevelAError(int c, std::string msg)
      : code(c),
        message(std::move(msg))
  {
  }
};

/// @brief Mid-level error, convertible to LevelAError via conversion operator.
/// NOT derived from LevelAError.
struct LevelBError
{
  int code;
  std::string message;
  std::string source;

  LevelBError(int c, std::string msg, std::string src)
      : code(c),
        message(std::move(msg)),
        source(std::move(src))
  {
  }

  operator LevelAError() const { return {code, message}; }
};

/// @brief Bottom-level error, convertible to both LevelBError AND LevelAError.
/// Direct conversion to LevelAError is required because C++ disallows
/// two implicit user-defined conversions in a chain.
struct LevelCError
{
  int code;
  std::string message;
  std::string source;
  std::string detail;

  LevelCError(
      int c,
      std::string msg,
      std::string src,
      std::string det)
      : code(c),
        message(std::move(msg)),
        source(std::move(src)),
        detail(std::move(det))
  {
  }

  operator LevelBError() const { return {code, message, source}; }
  operator LevelAError() const { return {code, message}; }
};

/// @brief Derived from LevelBError. Inherits conversion operator to
/// LevelAError. Used for "E3 derived from E2, E2 convertible to E1" tests.
struct DerivedLevelBError : LevelBError
{
  std::string extra;

  DerivedLevelBError(
      int c,
      std::string msg,
      std::string src,
      std::string ex)
      : LevelBError(c, std::move(msg), std::move(src)),
        extra(std::move(ex))
  {
  }
};

/// @brief Convertible to NetworkError (which derives from BaseError).
/// Implicitly convertible to BaseError via: 1 user-defined conversion
/// (->NetworkError) + 1 standard conversion (->BaseError).
/// NOT derived from NetworkError or BaseError.
///
/// @note operator NetworkError() is declared here but defined in
/// TestHelpers.hpp after NetworkError is available.
struct ConvertibleToNetworkError
{
  int code;
  std::string message;
  std::string endpoint;
  std::string extra;

  ConvertibleToNetworkError(
      int c,
      std::string msg,
      std::string ep,
      std::string ex)
      : code(c),
        message(std::move(msg)),
        endpoint(std::move(ep)),
        extra(std::move(ex))
  {
  }

  inline operator NetworkError() const;
};
