// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include "TestHelpers.hpp"

TEST(EitherVoidType, UNIT_011_OkAndVoidConstants)
{
  RecordProperty("id", "0.01-UNIT-011");
  RecordProperty("desc", "Either<Void, Error> works with OK and VOID constants");

  Either<Void, std::string> e1{OK};
  EXPECT_FALSE(e1.error());

  Either<Void, std::string> e2{VOID};
  EXPECT_FALSE(e2.error());
}

TEST(EitherVoidType, UNIT_012_VoidWithError)
{
  RecordProperty("id", "0.01-UNIT-012");
  RecordProperty("desc", "Either<Void, Error> correctly holds errors");

  Either<Void, std::string> e{std::string("validation error")};
  ASSERT_TRUE(e.error());
  EXPECT_EQ(*e.error(), "validation error");
}
