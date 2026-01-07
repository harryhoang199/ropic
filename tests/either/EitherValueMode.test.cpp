// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)
TEST(EitherValueMode, UNIT_001_ErrorConstructor)
{
  RecordProperty("id", "0.01-UNIT-001");
  RecordProperty(
      "desc", "Error constructor returns valid error and empty data");

  Either<int, std::string> e{std::string("error message")};
  ASSERT_TRUE(e.done());
  EXPECT_TRUE(e.error());
  EXPECT_FALSE(e.data());
  EXPECT_EQ(*e.error(), "error message");
}

TEST(EitherValueMode, UNIT_002_DataConstructor)
{
  RecordProperty("id", "0.01-UNIT-002");
  RecordProperty("desc", "Data constructor returns valid data and empty error");

  Either<int, std::string> e{42};
  ASSERT_TRUE(e.done());
  EXPECT_TRUE(e.data());
  EXPECT_FALSE(e.error());
  EXPECT_EQ(*e.data(), 42);
}

TEST(EitherValueMode, UNIT_003_ComplexTypes)
{
  RecordProperty("id", "0.01-UNIT-003");
  RecordProperty("desc", "Complex struct types for data and error");

  Either<TestData, std::string> dataEither{
      TestData{.value = 100, .name = "test name"}};
  ASSERT_TRUE(dataEither.done());
  ASSERT_TRUE(dataEither.data());
  EXPECT_EQ(dataEither.data()->value, 100);
  EXPECT_EQ(dataEither.data()->name, "test name");

  Either<int, TestError> errorEither{
      TestError{.code = 404, .message = "not found"}};
  ASSERT_TRUE(errorEither.done());
  ASSERT_TRUE(errorEither.error());
  EXPECT_EQ(errorEither.error()->code, 404);
  EXPECT_EQ(errorEither.error()->message, "not found");
}

TEST(EitherValueMode, UNIT_004_AccessorsSamePointer)
{
  RecordProperty("id", "0.01-UNIT-004");
  RecordProperty("desc", "Multiple accessor calls return same pointer");

  Either<int, std::string> dataE{42};
  ASSERT_TRUE(dataE.done());
  EXPECT_EQ(dataE.data().get(), dataE.data().get());
  EXPECT_EQ(dataE.data().get(), dataE.data().get());

  Either<int, std::string> errorE{std::string("err")};
  ASSERT_TRUE(errorE.done());
  EXPECT_EQ(errorE.error().get(), errorE.error().get());
  EXPECT_EQ(errorE.error().get(), errorE.error().get());
}

TEST(EitherValueMode, UNIT_005_AccessorsAnyOrder)
{
  RecordProperty("id", "0.01-UNIT-005");
  RecordProperty("desc", "error() and data() can be called in any order");

  Either<int, std::string> e1{42};
  ASSERT_TRUE(e1.done());
  EXPECT_FALSE(e1.error());
  EXPECT_TRUE(e1.data());
  EXPECT_FALSE(e1.error());
  EXPECT_TRUE(e1.data());

  Either<int, std::string> e2{std::string("err")};
  ASSERT_TRUE(e2.done());
  EXPECT_TRUE(e2.error());
  EXPECT_FALSE(e2.data());
  EXPECT_TRUE(e2.error());
  EXPECT_FALSE(e2.data());
}

TEST(EitherValueMode, UNIT_006_ConstDataAccessor)
{
  RecordProperty("id", "0.01-UNIT-006");
  RecordProperty(
      "desc", "data() const returns Borrower<const DATA> with correct value");

  const Either<int, std::string> e{42};
  ASSERT_TRUE(e.done());
  EXPECT_TRUE(e.data());
  EXPECT_FALSE(e.error());
  EXPECT_EQ(*e.data(), 42);

  // Verify const-correctness: Borrower<const int> should not allow mutation
  static_assert(
      std::is_same_v<decltype(*e.data()), int const&>,
      "data() const should return reference to const");
}

TEST(EitherValueMode, UNIT_007_ConstErrorAccessor)
{
  RecordProperty("id", "0.01-UNIT-007");
  RecordProperty(
      "desc", "error() const returns Borrower<const ERROR> with correct value");

  const Either<int, std::string> e{std::string("const error")};
  ASSERT_TRUE(e.done());
  EXPECT_TRUE(e.error());
  EXPECT_FALSE(e.data());
  EXPECT_EQ(*e.error(), "const error");

  // Verify const-correctness: Borrower<const string> should not allow mutation
  static_assert(
      std::is_same_v<decltype(*e.error()), std::string const&>,
      "error() const should return reference to const");
}

TEST(EitherValueMode, UNIT_008_ConstAccessorsComplexTypes)
{
  RecordProperty("id", "0.01-UNIT-008");
  RecordProperty("desc", "const accessors work with complex struct types");

  const Either<TestData, TestError> dataEither{
      TestData{.value = 200, .name = "const data"}};
  ASSERT_TRUE(dataEither.done());
  ASSERT_TRUE(dataEither.data());
  EXPECT_EQ(dataEither.data()->value, 200);
  EXPECT_EQ(dataEither.data()->name, "const data");

  const Either<TestData, TestError> errorEither{
      TestError{.code = 500, .message = "const error"}};
  ASSERT_TRUE(errorEither.done());
  ASSERT_TRUE(errorEither.error());
  EXPECT_EQ(errorEither.error()->code, 500);
  EXPECT_EQ(errorEither.error()->message, "const error");
}

TEST(EitherValueMode, UNIT_009_ConstAccessorPointerConsistency)
{
  RecordProperty("id", "0.01-UNIT-009");
  RecordProperty(
      "desc", "const accessor returns same pointer on multiple calls");

  const Either<int, std::string> dataE{99};
  EXPECT_EQ(dataE.data().get(), dataE.data().get());

  const Either<int, std::string> errorE{std::string("err")};
  EXPECT_EQ(errorE.error().get(), errorE.error().get());
}
// NOLINTEND(readability-magic-numbers)
