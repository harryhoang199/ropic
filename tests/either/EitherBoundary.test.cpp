// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)
TEST(EitherBoundary, UNIT_025_IntegerBoundaries)
{
  RecordProperty("id", "0.01-UNIT-025");
  RecordProperty("desc", "INT_MIN and INT_MAX as data values");

  Either<int, std::string> minE{INT_MIN};
  ASSERT_TRUE(minE.done());
  ASSERT_TRUE(minE.data());
  EXPECT_EQ(*minE.data(), INT_MIN);

  Either<int, std::string> maxE{INT_MAX};
  ASSERT_TRUE(maxE.done());
  ASSERT_TRUE(maxE.data());
  EXPECT_EQ(*maxE.data(), INT_MAX);
}

TEST(EitherBoundary, UNIT_026_EmptyStrings)
{
  RecordProperty("id", "0.01-UNIT-026");
  RecordProperty("desc", "Empty string as data and error");

  Either<std::string, int> dataE{std::string("")};
  ASSERT_TRUE(dataE.done());
  ASSERT_TRUE(dataE.data());
  EXPECT_EQ(*dataE.data(), "");

  Either<int, std::string> errorE{std::string("")};
  ASSERT_TRUE(errorE.done());
  ASSERT_TRUE(errorE.error());
  EXPECT_EQ(*errorE.error(), "");
}

TEST(EitherBoundary, UNIT_027_LargeStruct)
{
  RecordProperty("id", "0.01-UNIT-027");
  RecordProperty("desc", "Large struct handled correctly");

  LargeStruct large;
  large.values.fill(42);
  large.name = "large structure";

  Either<LargeStruct, std::string> e{std::move(large)};
  ASSERT_TRUE(e.done());
  ASSERT_TRUE(e.data());
  EXPECT_EQ(e.data()->values[0], 42);
  EXPECT_EQ(e.data()->values[99], 42);
  EXPECT_EQ(e.data()->name, "large structure");
}

TEST(EitherBoundary, UNIT_028_CoawaitRvalueAndLvalue)
{
  RecordProperty("id", "0.01-UNIT-028");
  RecordProperty("desc", "co_await works on both rvalue and lvalue Either");

  auto rvalueResult = awaitAndAdd(returnData(10), 5);
  ASSERT_TRUE(rvalueResult.done());
  ASSERT_TRUE(rvalueResult.data());
  EXPECT_EQ(*rvalueResult.data(), 15);

  auto input = returnData(20);
  auto lvalueResult = awaitAndAdd(std::move(input), 5);
  ASSERT_TRUE(lvalueResult.done());
  ASSERT_TRUE(lvalueResult.data());
  EXPECT_EQ(*lvalueResult.data(), 25);
}
// NOLINTEND(readability-magic-numbers)
