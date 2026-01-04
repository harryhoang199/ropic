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
  EXPECT_TRUE(e.error());
  EXPECT_FALSE(e.data());
  EXPECT_EQ(*e.error(), "error message");
}

TEST(EitherValueMode, UNIT_002_DataConstructor)
{
  RecordProperty("id", "0.01-UNIT-002");
  RecordProperty("desc", "Data constructor returns valid data and empty error");

  Either<int, std::string> e{42};
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
  ASSERT_TRUE(dataEither.data());
  EXPECT_EQ(dataEither.data()->value, 100);
  EXPECT_EQ(dataEither.data()->name, "test name");

  Either<int, TestError> errorEither{
      TestError{.code = 404, .message = "not found"}};
  ASSERT_TRUE(errorEither.error());
  EXPECT_EQ(errorEither.error()->code, 404);
  EXPECT_EQ(errorEither.error()->message, "not found");
}

TEST(EitherValueMode, UNIT_004_AccessorsSamePointer)
{
  RecordProperty("id", "0.01-UNIT-004");
  RecordProperty("desc", "Multiple accessor calls return same pointer");

  Either<int, std::string> dataE{42};
  EXPECT_EQ(dataE.data().get(), dataE.data().get());
  EXPECT_EQ(dataE.data().get(), dataE.data().get());

  Either<int, std::string> errorE{std::string("err")};
  EXPECT_EQ(errorE.error().get(), errorE.error().get());
  EXPECT_EQ(errorE.error().get(), errorE.error().get());
}

TEST(EitherValueMode, UNIT_005_AccessorsAnyOrder)
{
  RecordProperty("id", "0.01-UNIT-005");
  RecordProperty("desc", "error() and data() can be called in any order");

  Either<int, std::string> e1{42};
  EXPECT_FALSE(e1.error());
  EXPECT_TRUE(e1.data());
  EXPECT_FALSE(e1.error());
  EXPECT_TRUE(e1.data());

  Either<int, std::string> e2{std::string("err")};
  EXPECT_TRUE(e2.error());
  EXPECT_FALSE(e2.data());
  EXPECT_TRUE(e2.error());
  EXPECT_FALSE(e2.data());
}
// NOLINTEND(readability-magic-numbers)
