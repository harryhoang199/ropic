// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

TEST(M1S00_EitherTestingMacros, ROPIC_TESTING_MODE_Enabled)
{
#ifdef ROPIC_TESTING_MODE
  SUCCEED() << "ROPIC_TESTING_MODE macro enabled.";
#else
  ADD_FAILURE() << "ROPIC_TESTING_MODE macro disabled.";
#endif
}