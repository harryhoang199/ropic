// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

// NOLINTBEGIN(readability-magic-numbers)

TEST(M1S00EitherTestingMacros, U01RopicTestingModeEnabled)
{
  RecordProperty("id", "M1-S00-U01");
  RecordProperty("ver", "0.04");
  RecordProperty("desc", "ROPIC_TESTING_MODE macro is defined at compile time");

#ifdef ROPIC_TESTING_MODE
  SUCCEED() << "ROPIC_TESTING_MODE macro enabled.";
#else
  FAIL() << "ROPIC_TESTING_MODE macro disabled.";
#endif
}

// NOLINTEND(readability-magic-numbers)
