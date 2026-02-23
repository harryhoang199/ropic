// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)

TEST(M1S12EitherCrossTypeCoawaitSuccess, U01DiffValueDerivedError)
{
  RecordProperty("id", "M1-S12-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await Either<int, NetworkError> in Either<double, BaseError> "
      "succeeds");

  auto returnNetworkData = [](int x) -> Either<int, NetworkError>
  { co_return x; };

  auto outer = [&returnNetworkData](int x) -> Either<double, BaseError>
  {
    int val = co_await returnNetworkData(x);
    co_return static_cast<double>(val) * 1.5;
  };

  auto result = outer(10);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_DOUBLE_EQ(*result.value(), 15.0);
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U02SameValueDerivedError)
{
  RecordProperty("id", "M1-S12-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await Either<int, NetworkError> in Either<int, BaseError> "
      "succeeds");

  auto returnNetworkData = [](int x) -> Either<int, NetworkError>
  { co_return x; };

  auto outer = [&returnNetworkData](int x) -> Either<int, BaseError>
  {
    int val = co_await returnNetworkData(x);
    co_return val + 10;
  };

  auto result = outer(32);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U03VoidInnerDerivedError)
{
  RecordProperty("id", "M1-S12-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await Either<Void, NetworkError> validation in "
      "Either<int, BaseError> succeeds");

  auto returnNetworkVoidOK = []() -> Either<Void, NetworkError>
  { co_return OK; };

  auto outer = [&returnNetworkVoidOK](int x) -> Either<int, BaseError>
  {
    co_await returnNetworkVoidOK();
    co_return x * 2;
  };

  auto result = outer(21);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), 42);
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U04DiffValueConvertibleError)
{
  RecordProperty("id", "M1-S12-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await Either<int, LevelBError> in Either<double, LevelAError> "
      "succeeds (E2 convertible to E1, not derived)");

  auto inner = [](int x) -> Either<int, LevelBError> { co_return x; };

  auto outer = [&inner](int x) -> Either<double, LevelAError>
  {
    int val = co_await inner(x);
    co_return static_cast<double>(val) * 2.5;
  };

  auto result = outer(8);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_DOUBLE_EQ(*result.value(), 20.0);
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U05SkipLevelAllConv)
{
  RecordProperty("id", "M1-S12-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await Either<int, LevelCError> directly in "
      "Either<double, LevelAError> succeeds "
      "(E3 conv E2, E2 conv E1, skip-level)");

  auto f3 = [](int x) -> Either<int, LevelCError> { co_return x; };

  auto f1 = [&f3](int x) -> Either<double, LevelAError>
  {
    int val = co_await f3(x);
    co_return static_cast<double>(val) + 0.5;
  };

  auto result = f1(7);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_DOUBLE_EQ(*result.value(), 7.5);
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U06SkipLevelDerivedThenConv)
{
  RecordProperty("id", "M1-S12-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await Either<int, DerivedLevelBError> directly in "
      "Either<double, LevelAError> succeeds "
      "(E3 derived from E2, E2 conv E1, skip-level)");

  auto f3 = [](int x) -> Either<int, DerivedLevelBError> { co_return x; };

  auto f1 = [&f3](int x) -> Either<double, LevelAError>
  {
    int val = co_await f3(x);
    co_return static_cast<double>(val) * 3.0;
  };

  auto result = f1(5);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_DOUBLE_EQ(*result.value(), 15.0);
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U07SkipLevelConvThenDerived)
{
  RecordProperty("id", "M1-S12-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "co_await Either<int, ConvertibleToNetworkError> directly in "
      "Either<std::string, BaseError> succeeds "
      "(E3 conv E2, E2 derived from E1, skip-level)");

  auto f3 = [](int x) -> Either<int, ConvertibleToNetworkError>
  { co_return x; };

  auto f1 = [&f3](int x) -> Either<std::string, BaseError>
  {
    int val = co_await f3(x);
    co_return std::to_string(val * 2);
  };

  auto result = f1(21);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), "42");
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U08ChainedFirstFails)
{
  RecordProperty("id", "M1-S12-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "First cross-type co_await fails, second never reached");

  auto returnNetworkData = [](int x) -> Either<int, NetworkError>
  { co_return x; };

  auto returnNetworkError = [](int code, std::string msg, std::string endpoint)
      -> Either<int, NetworkError>
  { co_return NetworkError{code, std::move(msg), std::move(endpoint)}; };

  auto outer = [&returnNetworkError,
                &returnNetworkData]() -> Either<std::string, BaseError>
  {
    int a = co_await returnNetworkError(500, "Server Error", "/api");
    int b = co_await returnNetworkData(a + 10);
    co_return std::to_string(b);
  };

  auto result = outer();
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->code, 500);
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U09ChainedSecondFails)
{
  RecordProperty("id", "M1-S12-U09");
  RecordProperty("ver", "0.05");
  RecordProperty("desc", "First cross-type co_await succeeds, second fails");

  auto returnNetworkData = [](int x) -> Either<int, NetworkError>
  { co_return x; };

  auto returnNetworkVoidError =
      [](int code,
         std::string msg,
         std::string endpoint) -> Either<Void, NetworkError>
  { co_return NetworkError{code, std::move(msg), std::move(endpoint)}; };

  auto outer = [&returnNetworkData,
                &returnNetworkVoidError]() -> Either<std::string, BaseError>
  {
    int a = co_await returnNetworkData(10);
    co_await returnNetworkVoidError(422, "Validation", "/submit");
    co_return std::to_string(a);
  };

  auto result = outer();
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.error());
  EXPECT_EQ(result.error()->describe(), "Validation at /submit");
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U10ChainedAllSucceed)
{
  RecordProperty("id", "M1-S12-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "Multiple cross-type co_awaits all succeed in same coroutine");

  auto returnNetworkData = [](int x) -> Either<int, NetworkError>
  { co_return x; };

  auto validatePositive = [](int x) -> Either<Void, BaseError>
  {
    if (x <= 0)
      co_return BaseError{400, "must be positive"};
    co_return OK;
  };

  auto outer = [&returnNetworkData,
                &validatePositive](int x) -> Either<std::string, BaseError>
  {
    int a = co_await returnNetworkData(x);
    co_await validatePositive(a);
    co_return std::to_string(a * 2);
  };

  auto result = outer(21);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), "42");
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U11ThreeLevelAllDerivedNested)
{
  RecordProperty("id", "M1-S12-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Values flow through multi-level cross-type nesting with "
      "ServiceError type hierarchy");

  auto level3 = [](int x) -> Either<double, ServiceError>
  { co_return static_cast<double>(x) * 1.5; };

  auto level2 = [&level3](int x) -> Either<int, NetworkError>
  {
    double d = co_await level3(x);
    co_return static_cast<int>(d) + 10;
  };

  auto level1 = [&level2](int x) -> Either<std::string, BaseError>
  {
    int val = co_await level2(x);
    co_return std::to_string(val);
  };

  auto result = level1(20);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), "40");
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U12NestedAllConv)
{
  RecordProperty("id", "M1-S12-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Values flow through nested f1->f2->f3 with all convertible "
      "(non-derived) error types");

  auto f3 = []() -> Either<Void, LevelCError> { co_return OK; };

  auto f2 = [&f3](int x) -> Either<int, LevelBError>
  {
    co_await f3();
    co_return x * 3;
  };

  auto f1 = [&f2](int x) -> Either<std::string, LevelAError>
  {
    int val = co_await f2(x);
    co_return std::to_string(val);
  };

  auto result = f1(7);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), "21");
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U13NestedConvAndDerived)
{
  RecordProperty("id", "M1-S12-U13");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Values flow through nested f1->f2->f3 with "
      "E2 conv E1, E3 derived from E2");

  auto f3 = [](int x) -> Either<int, DerivedLevelBError> { co_return x; };

  auto f2 = [&f3](int x) -> Either<double, LevelBError>
  {
    int val = co_await f3(x);
    co_return static_cast<double>(val) + 0.5;
  };

  auto f1 = [&f2](int x) -> Either<std::string, LevelAError>
  {
    double val = co_await f2(x);
    co_return std::to_string(static_cast<int>(val));
  };

  auto result = f1(10);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), "10");
}

TEST(M1S12EitherCrossTypeCoawaitSuccess, U14NestedDerivedThenConv)
{
  RecordProperty("id", "M1-S12-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Values flow through nested f1->f2->f3 with "
      "E2 derived from E1, E3 conv E2");

  auto f3 = [](double x) -> Either<double, ConvertibleToNetworkError>
  { co_return x; };

  auto f2 = [&f3](double x) -> Either<int, NetworkError>
  {
    double d = co_await f3(x);
    co_return static_cast<int>(d) + 5;
  };

  auto f1 = [&f2](double x) -> Either<std::string, BaseError>
  {
    int val = co_await f2(x);
    co_return std::to_string(val);
  };

  auto result = f1(3.7);
  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(*result.value(), "8");
}

// NOLINTEND(readability-magic-numbers)
