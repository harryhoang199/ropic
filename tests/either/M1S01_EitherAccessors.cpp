// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <array>
#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)

// =============================================================================
// Local Test Types (only used in this file)
// =============================================================================

struct TestData
{
  int value;
  std::string name;
  auto operator==(const TestData& other) const -> bool = default;
};

struct TestError
{
  int code;
  std::string message;
  auto operator==(const TestError& other) const -> bool = default;
};

struct LargeStruct
{
  std::array<int, 100> values;
  std::string name;
  auto operator==(const LargeStruct& other) const -> bool = default;
};

// =============================================================================
// Basic Value Tests
// =============================================================================

TEST(M1S01_EitherAccessors, U01_OkAndVoidConstants)
{
  RecordProperty("id", "M1-S01-U01");
  RecordProperty("ver", "0.01");
  RecordProperty(
      "desc", "Either<Void, Error> works with OK and VOID constants");

  Either<Void, std::string> e1 = returnOK();
  ASSERT_TRUE(e1.done());
  EXPECT_FALSE(e1.error());
  EXPECT_EQ(*(e1.value()), OK);

  Either<void, std::string> e2 = returnOK();
  ASSERT_TRUE(e2.done());
  EXPECT_FALSE(e2.error());
  EXPECT_EQ(*(e2.value()), VOID);
}

TEST(M1S01_EitherAccessors, U02_VoidWithError)
{
  RecordProperty("id", "M1-S01-U02");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Either<Void, Error> correctly holds errors");

  Either<Void, std::string> e = returnVoidError("validation error");
  ASSERT_TRUE(e.done());
  ASSERT_TRUE(e.error());
  ASSERT_FALSE(e.value());
  EXPECT_EQ(*e.error(), "validation error");
}

TEST(M1S01_EitherAccessors, U03_ComplexTypes)
{
  RecordProperty("id", "M1-S01-U03");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Complex struct types for data and error");

  auto makeTestData = [](int v, std::string n) -> Either<TestData, std::string>
  { co_return TestData{.value = v, .name = std::move(n)}; };

  auto makeTestError = [](int c, std::string m) -> Either<int, TestError>
  { co_return TestError{.code = c, .message = std::move(m)}; };

  Either<TestData, std::string> dataEither = makeTestData(100, "test name");
  ASSERT_TRUE(dataEither.done());
  ASSERT_TRUE(dataEither.value());
  EXPECT_EQ(dataEither.value()->value, 100);
  EXPECT_EQ(dataEither.value()->name, "test name");

  Either<int, TestError> errorEither = makeTestError(404, "not found");
  ASSERT_TRUE(errorEither.done());
  ASSERT_TRUE(errorEither.error());
  EXPECT_EQ(errorEither.error()->code, 404);
  EXPECT_EQ(errorEither.error()->message, "not found");
}

TEST(M1S01_EitherAccessors, U04_LargeStruct)
{
  RecordProperty("id", "M1-S01-U04");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Large struct handled correctly");

  auto makeLargeStruct = [](LargeStruct ls) -> Either<LargeStruct, std::string>
  { co_return ls; };

  LargeStruct large;
  large.values.fill(42);
  large.name = "large structure";

  Either<LargeStruct, std::string> e = makeLargeStruct(std::move(large));
  ASSERT_TRUE(e.done());
  ASSERT_TRUE(e.value());
  EXPECT_EQ(e.value()->values[0], 42);
  EXPECT_EQ(e.value()->values[99], 42);
  EXPECT_EQ(e.value()->name, "large structure");
}

// =============================================================================
// Boundary Value Tests
// =============================================================================

TEST(M1S01_EitherAccessors, U05_IntegerBoundaries)
{
  RecordProperty("id", "M1-S01-U05");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "INT_MIN and INT_MAX as data values");

  Either<int, std::string> minE = returnData(INT_MIN);
  ASSERT_TRUE(minE.done());
  ASSERT_TRUE(minE.value());
  EXPECT_EQ(*minE.value(), INT_MIN);

  Either<int, std::string> maxE = returnData(INT_MAX);
  ASSERT_TRUE(maxE.done());
  ASSERT_TRUE(maxE.value());
  EXPECT_EQ(*maxE.value(), INT_MAX);
}

TEST(M1S01_EitherAccessors, U06_EmptyStrings)
{
  RecordProperty("id", "M1-S01-U06");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Empty string as data and error");

  auto makeStringData = [](std::string s) -> Either<std::string, int>
  { co_return s; };

  Either<std::string, int> dataE = makeStringData("");
  ASSERT_TRUE(dataE.done());
  ASSERT_TRUE(dataE.value());
  EXPECT_EQ(*dataE.value(), "");

  Either<int, std::string> errorE = returnError("");
  ASSERT_TRUE(errorE.done());
  ASSERT_TRUE(errorE.error());
  EXPECT_EQ(*errorE.error(), "");
}

// =============================================================================
// Accessor Consistency Tests
// =============================================================================

TEST(M1S01_EitherAccessors, U07_AccessorsSamePointer)
{
  RecordProperty("id", "M1-S01-U07");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Multiple accessor calls return same pointer");

  Either<int, std::string> dataE = returnData(42);
  ASSERT_TRUE(dataE.done());
  EXPECT_EQ(dataE.value().get(), dataE.value().get());
  EXPECT_EQ(dataE.value().get(), dataE.value().get());

  Either<int, std::string> errorE = returnError("err");
  ASSERT_TRUE(errorE.done());
  EXPECT_EQ(errorE.error().get(), errorE.error().get());
  EXPECT_EQ(errorE.error().get(), errorE.error().get());
}

TEST(M1S01_EitherAccessors, U08_AccessorsAnyOrder)
{
  RecordProperty("id", "M1-S01-U08");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "error() and value() can be called in any order");

  Either<int, std::string> e1 = returnData(42);
  ASSERT_TRUE(e1.done());
  EXPECT_FALSE(e1.error());
  EXPECT_TRUE(e1.value());
  EXPECT_FALSE(e1.error());
  EXPECT_TRUE(e1.value());

  Either<int, std::string> e2 = returnError("err");
  ASSERT_TRUE(e2.done());
  EXPECT_TRUE(e2.error());
  EXPECT_FALSE(e2.value());
  EXPECT_TRUE(e2.error());
  EXPECT_FALSE(e2.value());
}

// =============================================================================
// Const Accessor Tests
// =============================================================================

TEST(M1S01_EitherAccessors, U09_ConstDataAccessor)
{
  RecordProperty("id", "M1-S01-U09");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "value() const returns Borrower<const VALUE> with correct value");

  const Either<int, std::string> e = returnData(42);
  ASSERT_TRUE(e.done());
  EXPECT_TRUE(e.value());
  EXPECT_FALSE(e.error());
  EXPECT_EQ(*e.value(), 42);
}

TEST(M1S01_EitherAccessors, U10_ConstErrorAccessor)
{
  RecordProperty("id", "M1-S01-U10");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "error() const returns Borrower<const ERROR> with correct value");

  const Either<int, std::string> e = returnError("const error");
  ASSERT_TRUE(e.done());
  EXPECT_TRUE(e.error());
  EXPECT_FALSE(e.value());
  EXPECT_EQ(*e.error(), "const error");
}

TEST(M1S01_EitherAccessors, U11_ConstAccessorsComplexTypes)
{
  RecordProperty("id", "M1-S01-U11");
  RecordProperty("ver", "0.02");
  RecordProperty("desc", "const accessors work with complex struct types");

  auto makeTestDataWithTestError =
      [](int v, std::string n) -> Either<TestData, TestError>
  { co_return TestData{.value = v, .name = std::move(n)}; };

  auto makeTestErrorWithTestData =
      [](int c, std::string m) -> Either<TestData, TestError>
  { co_return TestError{.code = c, .message = std::move(m)}; };

  const Either<TestData, TestError> dataEither =
      makeTestDataWithTestError(200, "const data");
  ASSERT_TRUE(dataEither.done());
  ASSERT_TRUE(dataEither.value());
  EXPECT_EQ(dataEither.value()->value, 200);
  EXPECT_EQ(dataEither.value()->name, "const data");

  const Either<TestData, TestError> errorEither =
      makeTestErrorWithTestData(500, "const error");
  ASSERT_TRUE(errorEither.done());
  ASSERT_TRUE(errorEither.error());
  EXPECT_EQ(errorEither.error()->code, 500);
  EXPECT_EQ(errorEither.error()->message, "const error");
}

TEST(M1S01_EitherAccessors, U12_ConstAccessorPointerConsistency)
{
  RecordProperty("id", "M1-S01-U12");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "const accessor returns same pointer on multiple calls");

  const Either<int, std::string> dataE = returnData(99);
  EXPECT_EQ(dataE.value().get(), dataE.value().get());

  const Either<int, std::string> errorE = returnError("err");
  EXPECT_EQ(errorE.error().get(), errorE.error().get());
}

// NOLINTEND(readability-magic-numbers)
