// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)

TEST(M1S04EitherCoReturnErrorVariants, U01BraceInitCoreturnError)
{
  RecordProperty("id", "M1-S04-U01");
  RecordProperty("ver", "0.03");
  RecordProperty("desc", "co_return {args...} constructs ERROR via brace-init");

  auto returnErrorContextBraceInit =
      [](int code,
         std::string src,
         std::string msg) -> Either<int, ErrorContext>
  { co_return {code, std::move(src), std::move(msg)}; };

  auto result = returnErrorContextBraceInit(404, "api", "Resource not found");

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, 404);
  EXPECT_EQ(result.error()->source, "api");
  EXPECT_EQ(result.error()->message, "Resource not found");
}

TEST(M1S04EitherCoReturnErrorVariants, U02PolymorphicErrorDirect)
{
  RecordProperty("id", "M1-S04-U02");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "co_return DerivedError{...} stores derived type correctly");

  auto returnNetworkError = [](int code,
                               std::string msg,
                               std::string endpoint) -> Either<int, BaseError>
  { co_return NetworkError{code, std::move(msg), std::move(endpoint)}; };

  auto result = returnNetworkError(500, "Connection failed", "/api/v1");

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, 500);
  EXPECT_EQ(result.error()->message, "Connection failed");
  EXPECT_EQ(result.error()->describe(), "Connection failed at /api/v1");
}

TEST(M1S04EitherCoReturnErrorVariants, U03PolymorphicErrorLvalue)
{
  RecordProperty("id", "M1-S04-U03");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return lvalue DerivedError preserves derived type via "
      "const& overload");

  NetworkError err{502, "Bad Gateway", "/api/v2"};
  auto returnNetworkErrorLvalue = [&err]() -> Either<int, BaseError>
  { co_return err; };

  auto result = returnNetworkErrorLvalue();

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, err.code);
  EXPECT_EQ(result.error()->message, err.message);
  EXPECT_EQ(result.error()->describe(), "Bad Gateway at /api/v2");
}

TEST(M1S04EitherCoReturnErrorVariants, U04ImmovableErrorLvalueFallback)
{
  RecordProperty("id", "M1-S04-U04");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return lvalue ImmovableNetworkError (no copy/move) falls "
      "back to const BaseError& overload, slicing to base type");

  ImmovableNetworkError err{503, "Service Unavailable", "/api/v3"};
  auto returnImmovableLvalue = [&err]() -> Either<int, BaseError>
  { co_return err; };

  auto result = returnImmovableLvalue();

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, 503);
  EXPECT_EQ(result.error()->message, "Service Unavailable");
  // Slicing occurred — derived type info is lost
  EXPECT_EQ(result.error()->describe(), "Service Unavailable");
}

TEST(M1S04EitherCoReturnErrorVariants, U05ImmovableErrorRvalueFallback)
{
  RecordProperty("id", "M1-S04-U05");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return rvalue ImmovableNetworkError (no copy/move) falls "
      "back to BaseError&& overload, slicing to base type");

  auto returnImmovableRvalue = []() -> Either<int, BaseError>
  { co_return ImmovableNetworkError{504, "Gateway Timeout", "/api/v4"}; };

  auto result = returnImmovableRvalue();

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, 504);
  EXPECT_EQ(result.error()->message, "Gateway Timeout");
  // Slicing occurred — derived type info is lost
  EXPECT_EQ(result.error()->describe(), "Gateway Timeout");
}

TEST(M1S04EitherCoReturnErrorVariants, U06PolymorphicErrorBaseType)
{
  RecordProperty("id", "M1-S04-U06");
  RecordProperty("ver", "0.03");
  RecordProperty("desc", "co_return BaseError{...} also works correctly");

  auto returnBaseError = [](int code, std::string msg) -> Either<int, BaseError>
  { co_return BaseError{code, std::move(msg)}; };

  auto result = returnBaseError(400, "Bad request");

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, 400);
  EXPECT_EQ(result.error()->describe(), "Bad request");
}

TEST(M1S04EitherCoReturnErrorVariants, U07PolymorphicErrorPropagation)
{
  RecordProperty("id", "M1-S04-U07");
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
  ASSERT_EQ(errorResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(errorResult.error());
  EXPECT_EQ(errorResult.error()->describe(), "Connection failed at /api/v1");

  auto successResult = chainedPolymorphicError(false);
  ASSERT_EQ(successResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(successResult.value());
  EXPECT_EQ(*successResult.value(), 42);
}

TEST(M1S04EitherCoReturnErrorVariants, U08ConstRefCoreturnError)
{
  RecordProperty("id", "M1-S04-U08");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return const ERROR& invokes return_value(ERROR const&) "
      "overload");

  const ErrorContext err{401, "auth", "Unauthorized"};
  auto returnConstErrorContext = [&err]() -> Either<int, ErrorContext>
  { co_return err; };

  auto result = returnConstErrorContext();

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, err.code);
  EXPECT_EQ(result.error()->source, err.source);
  EXPECT_EQ(result.error()->message, err.message);
}

TEST(M1S04EitherCoReturnErrorVariants, U09TupleImmovableError)
{
  RecordProperty("id", "M1-S04-U09");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return std::make_tuple constructs immovable ERROR in-place "
      "without move");

  auto returnImmovableErrorExplicitTuple =
      [](int code,
         std::string src,
         std::string msg) -> Either<int, ImmovableError>
  { co_return std::make_tuple(code, std::move(src), std::move(msg)); };

  auto result =
      returnImmovableErrorExplicitTuple(503, "server", "Service unavailable");

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, 503);
  EXPECT_EQ(result.error()->source, "server");
  EXPECT_EQ(result.error()->message, "Service unavailable");
}

TEST(M1S04EitherCoReturnErrorVariants, U10ErrorDestructorOnEitherDestroy)
{
  RecordProperty("id", "M1-S04-U10");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "Destructor of ERROR is called when Either is destroyed");

  using EDT = ErrorDestructorTracker<"M1-S04-U10">;

  auto returnErr = [](int code, std::string msg) -> Either<int, EDT>
  { co_return EDT{code, std::move(msg)}; };

  EDT::reset();
  ASSERT_EQ(EDT::s_destructorCount, 0);

  {
    auto result = returnErr(500, "Internal error");
    ASSERT_EQ(result.state(), ropic::CoroState::DONE);
    ASSERT_TRUE(result.error());
    EXPECT_EQ(result.error()->code, 500);
    EXPECT_EQ(result.error()->message, "Internal error");
  }

  EXPECT_GE(EDT::s_destructorCount, 1);
}

TEST(M1S04EitherCoReturnErrorVariants, U11DerivedErrorDestructorOnEitherDestroy)
{
  RecordProperty("id", "M1-S04-U11");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "Destructor of DERIVED_ERR is called when Either is "
      "destroyed");

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
    ASSERT_EQ(result.state(), ropic::CoroState::DONE);
    ASSERT_TRUE(result.error());
    EXPECT_EQ(result.error()->code, 404);
    EXPECT_EQ(result.error()->describe(), "Not found: /api/users/123");
  }

  EXPECT_GE(EDT::s_destructorCount, 1);
  EXPECT_GE(DEDT::s_derivedDestructorCount, 1);
}

TEST(M1S04EitherCoReturnErrorVariants, U12NoErrorDestructorOnValue)
{
  RecordProperty("id", "M1-S04-U12");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "ERROR destructor is NOT called when Either contains value");

  using EDT = ErrorDestructorTracker<"M1-S04-U12">;

  auto returnValue = [](int val) -> Either<int, EDT> { co_return val; };

  EDT::reset();
  ASSERT_EQ(EDT::s_destructorCount, 0);

  {
    auto result = returnValue(42);
    ASSERT_EQ(result.state(), ropic::CoroState::DONE);
    ASSERT_TRUE(result.value());
    EXPECT_EQ(*result.value(), 42);
  }

  EXPECT_EQ(EDT::s_destructorCount, 0);
}

// NOLINTEND(readability-magic-numbers)
