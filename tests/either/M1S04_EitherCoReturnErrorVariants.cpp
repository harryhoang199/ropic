// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)

TEST(M1S04_EitherCoReturnErrorVariants, U01_BraceInitCoreturnError)
{
  RecordProperty("id", "M1-S12-U01");
  RecordProperty("ver", "0.03");
  RecordProperty("desc", "co_return {args...} constructs ERROR via brace-init");

  auto returnErrorContextBraceInit =
      [](int code,
         std::string src,
         std::string msg) -> Either<int, ErrorContext>
  { co_return {code, std::move(src), std::move(msg)}; };

  auto result = returnErrorContextBraceInit(404, "api", "Resource not found");

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 404);
  EXPECT_EQ(result.error()->source, "api");
  EXPECT_EQ(result.error()->message, "Resource not found");
}

TEST(M1S04_EitherCoReturnErrorVariants, U02_PolymorphicErrorDirect)
{
  RecordProperty("id", "M1-S12-U02");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "co_return DerivedError{...} stores derived type correctly");

  auto returnNetworkError = [](int code,
                               std::string msg,
                               std::string endpoint) -> Either<int, BaseError>
  { co_return NetworkError{code, std::move(msg), std::move(endpoint)}; };

  auto result = returnNetworkError(500, "Connection failed", "/api/v1");

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 500);
  EXPECT_EQ(result.error()->message, "Connection failed");

  // Verify polymorphic behavior via virtual function
  EXPECT_EQ(result.error()->describe(), "Connection failed at /api/v1");
}

TEST(M1S04_EitherCoReturnErrorVariants, U03_PolymorphicErrorLvalue)
{
  RecordProperty("id", "M1-S12-U03");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return lvalue DerivedError preserves derived type via const& "
      "overload");
  NetworkError err{502, "Bad Gateway", "/api/v2"};
  auto returnNetworkErrorLvalue = [&err]() -> Either<int, BaseError>
  { co_return err; };

  auto result = returnNetworkErrorLvalue();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, err.code);
  EXPECT_EQ(result.error()->message, err.message);

  // CRITICAL TEST: Verify polymorphic behavior is preserved for lvalue
  // If this fails with "Bad Gateway" instead of "Bad Gateway at /api/v2",
  // it means the derived type was sliced to base ERROR
  EXPECT_EQ(result.error()->describe(), "Bad Gateway at /api/v2");
}

TEST(M1S04_EitherCoReturnErrorVariants, U04_ImmovableErrorLvalueFallback)
{
  RecordProperty("id", "M1-S12-U04");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return lvalue ImmovableNetworkError (no copy/move) falls back to "
      "const BaseError& overload, slicing to base type");

  // Note: ImmovableNetworkError has deleted copy/move constructors
  // So the template return_value(DERIVED_ERR&&) cannot be used
  // Instead, it falls back to return_value(ERROR const&) which requires
  // converting to BaseError, resulting in object slicing
  ImmovableNetworkError err{503, "Service Unavailable", "/api/v3"};
  auto returnImmovableLvalue = [&err]() -> Either<int, BaseError>
  { co_return err; };

  auto result = returnImmovableLvalue();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 503);
  EXPECT_EQ(result.error()->message, "Service Unavailable");

  // CRITICAL: Because ImmovableNetworkError cannot be copied/moved,
  // it must be converted to BaseError const&, causing object slicing.
  // The derived type information (endpoint, virtual describe) is lost.
  EXPECT_EQ(result.error()->describe(), "Service Unavailable");
  // NOT "Service Unavailable [immovable] at /api/v3" - slicing occurred
}

TEST(M1S04_EitherCoReturnErrorVariants, U05_ImmovableErrorRvalueFallback)
{
  RecordProperty("id", "M1-S12-U05");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return rvalue ImmovableNetworkError (no copy/move) falls back to "
      "BaseError&& overload, slicing to base type");

  auto returnImmovableRvalue = []() -> Either<int, BaseError>
  {
    // Create rvalue directly in co_return
    // Cannot use template overload due to deleted move constructor
    // Falls back to return_value(ERROR&&) via BaseError conversion
    co_return ImmovableNetworkError{504, "Gateway Timeout", "/api/v4"};
  };

  auto result = returnImmovableRvalue();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 504);
  EXPECT_EQ(result.error()->message, "Gateway Timeout");

  // CRITICAL: Object slicing occurs during conversion to BaseError&&
  // The derived type's virtual function and extra data are lost
  EXPECT_EQ(result.error()->describe(), "Gateway Timeout");
  // NOT "Gateway Timeout [immovable] at /api/v4" - slicing occurred
}

TEST(M1S04_EitherCoReturnErrorVariants, U06_PolymorphicErrorBaseType)
{
  RecordProperty("id", "M1-S12-U06");
  RecordProperty("ver", "0.03");
  RecordProperty("desc", "co_return BaseError{...} also works correctly");

  auto returnBaseError = [](int code, std::string msg) -> Either<int, BaseError>
  { co_return BaseError{code, std::move(msg)}; };

  auto result = returnBaseError(400, "Bad request");

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 400);
  EXPECT_EQ(result.error()->describe(), "Bad request");
}

TEST(M1S04_EitherCoReturnErrorVariants, U07_PolymorphicErrorPropagation)
{
  RecordProperty("id", "M1-S12-U07");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "Derived error propagates correctly through co_await chain");

  auto returnNetworkError = [](int code,
                               std::string msg,
                               std::string endpoint) -> Either<int, BaseError>
  { co_return NetworkError{code, std::move(msg), std::move(endpoint)}; };

  auto chainedPolymorphicError =
      [&returnNetworkError](bool shouldFail) -> Either<int, BaseError>
  {
    if (shouldFail)
    {
      int val =
          co_await returnNetworkError(500, "Connection failed", "/api/v1");
      co_return val;
    }
    co_return 42;
  };

  auto errorResult = chainedPolymorphicError(true);
  ASSERT_TRUE(errorResult.done());
  ASSERT_TRUE(errorResult.error());
  ASSERT_FALSE(errorResult.value());
  EXPECT_EQ(errorResult.error()->describe(), "Connection failed at /api/v1");

  auto successResult = chainedPolymorphicError(false);
  ASSERT_TRUE(successResult.done());
  ASSERT_TRUE(successResult.value());
  ASSERT_FALSE(successResult.error());
  EXPECT_EQ(*successResult.value(), 42);
}

TEST(M1S04_EitherCoReturnErrorVariants, U08_ConstRefCoreturnError)
{
  RecordProperty("id", "M1-S12-U08");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return const ERROR& invokes return_value(ERROR const&) overload");

  const ErrorContext err{401, "auth", "Unauthorized"};
  auto returnConstErrorContext = [&err]() -> Either<int, ErrorContext>
  { co_return err; };

  auto result = returnConstErrorContext();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, err.code);
  EXPECT_EQ(result.error()->source, err.source);
  EXPECT_EQ(result.error()->message, err.message);
}

TEST(M1S04_EitherCoReturnErrorVariants, U09_TupleImmovableError)
{
  RecordProperty("id", "M1-S12-U09");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return std::make_tuple constructs immovable ERROR in-place without "
      "move");

  auto returnImmovableErrorExplicitTuple =
      [](int code,
         std::string src,
         std::string msg) -> Either<int, ImmovableError>
  { co_return std::make_tuple(code, std::move(src), std::move(msg)); };

  auto result =
      returnImmovableErrorExplicitTuple(503, "server", "Service unavailable");

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 503);
  EXPECT_EQ(result.error()->source, "server");
  EXPECT_EQ(result.error()->message, "Service unavailable");
}

TEST(M1S04_EitherCoReturnErrorVariants, U10_ErrorDestructorOnEitherDestroy)
{
  RecordProperty("id", "M1-S12-U10");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "Destructor of ERROR is called when Either is destroyed");

  using EDT = ErrorDestructorTracker<"M1-S04-U10">;

  auto returnError = [](int code, std::string msg) -> Either<int, EDT>
  { co_return EDT{code, std::move(msg)}; };

  EDT::reset();
  ASSERT_EQ(EDT::s_destructorCount, 0);

  {
    auto result = returnError(500, "Internal error");
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    ASSERT_FALSE(result.value());
    EXPECT_EQ(result.error()->code, 500);
    EXPECT_EQ(result.error()->message, "Internal error");
  }

  // After Either goes out of scope, error destructor should have been called
  EXPECT_GE(EDT::s_destructorCount, 1);
}

TEST(
    M1S04_EitherCoReturnErrorVariants,
    U11_DerivedErrorDestructorOnEitherDestroy)
{
  RecordProperty("id", "M1-S12-U11");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "Destructor of DERIVED_ERR is called when Either is destroyed");

  using EDT = ErrorDestructorTracker<"M1-S04-U11">;
  using DEDT = DerivedErrorDestructorTracker<"M1-S04-U11">;

  auto returnDerivedError =
      [](int code, std::string msg, std::string detail) -> Either<int, EDT>
  { co_return DEDT{code, std::move(msg), std::move(detail)}; };

  DEDT::resetDerived();
  ASSERT_EQ(EDT::s_destructorCount, 0);
  ASSERT_EQ(DEDT::s_derivedDestructorCount, 0);

  {
    auto result = returnDerivedError(404, "Not found", "/api/users/123");
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    ASSERT_FALSE(result.value());
    EXPECT_EQ(result.error()->code, 404);
    EXPECT_EQ(result.error()->describe(), "Not found: /api/users/123");
  }

  // Both base and derived destructors should have been called
  EXPECT_GE(EDT::s_destructorCount, 1);
  EXPECT_GE(DEDT::s_derivedDestructorCount, 1);
}

TEST(M1S04_EitherCoReturnErrorVariants, U12_NoErrorDestructorOnValue)
{
  RecordProperty("id", "M1-S12-U12");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "ERROR destructor is NOT called when Either contains value");

  using EDT = ErrorDestructorTracker<"M1-S04-U12">;

  auto returnValue = [](int val) -> Either<int, EDT> { co_return val; };

  EDT::reset();
  ASSERT_EQ(EDT::s_destructorCount, 0);

  {
    auto result = returnValue(42);
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.value());
    ASSERT_FALSE(result.error());
    EXPECT_EQ(*result.value(), 42);
  }

  // No ErrorDestructorTracker was ever constructed, so count should be 0
  EXPECT_EQ(EDT::s_destructorCount, 0);
}

// NOLINTEND(readability-magic-numbers)
