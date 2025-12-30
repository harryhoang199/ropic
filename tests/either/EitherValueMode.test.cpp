// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include "TestHelpers.hpp"

TEST(EitherValueMode, UNIT_001_ErrorConstructor)
{
  RecordProperty("id", "0.01-UNIT-001");
  RecordProperty("desc", "Error constructor returns valid error and empty data");

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

  Either<TestData, std::string> dataEither{TestData{100, "test name"}};
  ASSERT_TRUE(dataEither.data());
  EXPECT_EQ(dataEither.data()->value, 100);
  EXPECT_EQ(dataEither.data()->name, "test name");

  Either<int, TestError> errorEither{TestError{404, "not found"}};
  ASSERT_TRUE(errorEither.error());
  EXPECT_EQ(errorEither.error()->code, 404);
  EXPECT_EQ(errorEither.error()->message, "not found");
}

TEST(EitherValueMode, UNIT_004_AccessorsSamePointer)
{
  GTEST_SKIP() << "Skipping this test because functionality is not yet implemented";

  // RecordProperty("id", "0.01-UNIT-004");
  // RecordProperty("desc", "Multiple accessor calls return same pointer");

  // // Either<int, std::string> dataE{42};
  // // EXPECT_EQ(dataE.data(), dataE.data());
  // // EXPECT_EQ(dataE.data(), dataE.data());

  // // Either<int, std::string> errorE{std::string("err")};
  // // EXPECT_EQ(errorE.error(), errorE.error());
  // // EXPECT_EQ(errorE.error(), errorE.error());
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
