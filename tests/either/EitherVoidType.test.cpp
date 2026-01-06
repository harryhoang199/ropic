// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "ropic.hpp"

using namespace ropic;

TEST(EitherVoidType, UNIT_011_OkAndVoidConstants)
{
  RecordProperty("id", "0.01-UNIT-011");
  RecordProperty(
      "desc", "Either<Void, Error> works with OK and VOID constants");

  Either<void, std::string> e1{OK};
  ASSERT_TRUE(e1.done());
  EXPECT_FALSE(e1.error());

  Either<Void, std::string> e2{VOID};
  ASSERT_TRUE(e2.done());
  EXPECT_FALSE(e2.error());
}

TEST(EitherVoidType, UNIT_012_VoidWithError)
{
  RecordProperty("id", "0.01-UNIT-012");
  RecordProperty("desc", "Either<Void, Error> correctly holds errors");

  Either<void, std::string> e{std::string("validation error")};
  ASSERT_TRUE(e.done());
  ASSERT_TRUE(e.error());
  EXPECT_EQ(*e.error(), "validation error");
}
