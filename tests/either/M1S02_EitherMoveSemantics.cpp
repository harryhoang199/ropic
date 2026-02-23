// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)

TEST(M1S02EitherMoveSemantics, U01MoveConstruct)
{
  RecordProperty("id", "M1-S02-U01");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Move constructor transfers ownership correctly");

  Either<int, std::string> srcErr = returnError("error");
  ASSERT_EQ(srcErr.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(srcErr.error());
  Either<int, std::string> dstErr{std::move(srcErr)};
  EXPECT_EQ(srcErr.state(), ropic::CoroState::UNDEFINED);

  ASSERT_EQ(dstErr.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(dstErr.error());
  EXPECT_EQ(*dstErr.error(), "error");

  Either<int, std::string> srcData = returnData(42);
  ASSERT_EQ(srcData.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(srcData.value());
  Either<int, std::string> dstData{std::move(srcData)};
  EXPECT_EQ(srcData.state(), ropic::CoroState::UNDEFINED);
  ASSERT_EQ(dstData.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(dstData.value());
  EXPECT_EQ(*dstData.value(), 42);
}

TEST(M1S02EitherMoveSemantics, U02MoveAssign)
{
  RecordProperty("id", "M1-S02-U02");
  RecordProperty("ver", "0.01");
  RecordProperty(
      "desc", "Move assignment overwrites data with error and vice versa");

  Either<int, std::string> src1 = returnError("new error");
  Either<int, std::string> dst1 = returnData(100);
  ASSERT_EQ(src1.state(), ropic::CoroState::DONE);
  ASSERT_EQ(dst1.state(), ropic::CoroState::DONE);
  dst1 = std::move(src1);
  EXPECT_EQ(src1.state(), ropic::CoroState::UNDEFINED);

  ASSERT_EQ(dst1.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(dst1.error());
  EXPECT_EQ(*dst1.error(), "new error");

  Either<int, std::string> src2 = returnData(200);
  Either<int, std::string> dst2 = returnError("old error");
  ASSERT_EQ(src2.state(), ropic::CoroState::DONE);
  ASSERT_EQ(dst2.state(), ropic::CoroState::DONE);
  dst2 = std::move(src2);
  EXPECT_EQ(src2.state(), ropic::CoroState::UNDEFINED);

  ASSERT_EQ(dst2.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(dst2.value());
  EXPECT_EQ(*dst2.value(), 200);
}

TEST(M1S02EitherMoveSemantics, U03SelfMoveAssign)
{
  RecordProperty("id", "M1-S02-U03");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Self-move-assignment does not crash");

  Either<int, std::string> e = returnData(42);
  // Intentionally testing self-move-assignment behavior
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wself-move"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wself-move"
#endif
  e = std::move(e);
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
  EXPECT_EQ(e.state(), ropic::CoroState::DONE);
}

TEST(M1S02EitherMoveSemantics, U04MoveFromRvalueRef)
{
  RecordProperty("id", "M1-S02-U04");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Move construct from rvalue reference");

  Either<int, std::string>&& src = returnData(42);
  Either<int, std::string> dst{std::move(src)};
  ASSERT_EQ(dst.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(dst.value());
  EXPECT_EQ(*dst.value(), 42);
}

TEST(M1S02EitherMoveSemantics, U05ZeroCopies)
{
  RecordProperty("id", "M1-S02-U05");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Move operations use move semantics, zero copies");

  using MT = MoveTracker<"M1-S02-U05">;

  MT::reset();
  Either<MT, std::string> src = returnMoveTracker<"M1-S02-U05">(42);
  ASSERT_EQ(src.state(), ropic::CoroState::DONE);
  Either<MT, std::string> dst{std::move(src)};
  EXPECT_EQ(src.state(), ropic::CoroState::UNDEFINED);
  EXPECT_EQ(MT::s_copyCount, 0);
  EXPECT_GE(MT::s_moveCount, 1);
  ASSERT_EQ(dst.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(dst.value());
  EXPECT_EQ(dst.value()->value, 42);

  MT::reset();
  Either<MT, std::string> src2 = returnMoveTracker<"M1-S02-U05">(42);
  Either<MT, std::string> dst2 = returnMoveTracker<"M1-S02-U05">(0);
  ASSERT_EQ(src2.state(), ropic::CoroState::DONE);
  ASSERT_EQ(dst2.state(), ropic::CoroState::DONE);
  dst2 = std::move(src2);
  EXPECT_EQ(src2.state(), ropic::CoroState::UNDEFINED);
  ASSERT_EQ(dst2.state(), ropic::CoroState::DONE);
  EXPECT_EQ(MT::s_copyCount, 0);
  EXPECT_GE(MT::s_moveCount, 1);

  MT::reset();
  Either<int, MT> errSrc = returnIntWithMoveTrackerError<"M1-S02-U05">(true);
  ASSERT_EQ(errSrc.state(), ropic::CoroState::DONE);
  Either<int, MT> errDst{std::move(errSrc)};
  EXPECT_EQ(errSrc.state(), ropic::CoroState::UNDEFINED);
  EXPECT_EQ(MT::s_copyCount, 0);
  ASSERT_EQ(errDst.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(errDst.error());
  EXPECT_EQ(errDst.error()->value, -1);
}

// =============================================================================
// Death Tests (separate suite, run first in single-threaded context)
// =============================================================================

TEST(M1S02EitherMoveSemantics, U06MovedFromErrorAccess)
{
  RecordProperty("id", "M1-S02-U06");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Accessing error()/value() on moved-from Either asserts");

  GTEST_FLAG_SET(death_test_style, "threadsafe");

  [[maybe_unused]]
  Either<int, std::string> src = returnError("error");
  Either<int, std::string> dst{std::move(src)};
  ASSERT_EQ(src.state(), ropic::CoroState::UNDEFINED);

  EXPECT_DEBUG_DEATH((void)src.error(), "");
  EXPECT_DEBUG_DEATH((void)src.value(), "");
}

TEST(M1S02EitherMoveSemanticsDeathTest, U07MovedFromDataAccess)
{
  RecordProperty("id", "M1-S02-U07");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Accessing value()/error() on moved-from data Either asserts");

  GTEST_FLAG_SET(death_test_style, "threadsafe");

  [[maybe_unused]]
  Either<int, std::string> src = returnData(42);
  Either<int, std::string> dst{std::move(src)};
  ASSERT_EQ(src.state(), ropic::CoroState::UNDEFINED);

  EXPECT_DEBUG_DEATH((void)src.value(), "");
  EXPECT_DEBUG_DEATH((void)src.error(), "");
}

TEST(M1S02EitherMoveSemanticsDeathTest, U08MoveAssignedFromAccess)
{
  RecordProperty("id", "M1-S02-U08");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc", "Accessing error()/value() after move-assignment asserts");

  GTEST_FLAG_SET(death_test_style, "threadsafe");

  [[maybe_unused]]
  Either<int, std::string> src1 = returnError("new error");
  Either<int, std::string> dst1 = returnData(100);
  dst1 = std::move(src1);
  ASSERT_EQ(src1.state(), ropic::CoroState::UNDEFINED);

  EXPECT_DEBUG_DEATH((void)src1.error(), "");
  EXPECT_DEBUG_DEATH((void)src1.value(), "");

  [[maybe_unused]]
  Either<int, std::string> src2 = returnData(200);
  Either<int, std::string> dst2 = returnError("old error");
  dst2 = std::move(src2);
  ASSERT_EQ(src2.state(), ropic::CoroState::UNDEFINED);

  EXPECT_DEBUG_DEATH((void)src2.error(), "");
  EXPECT_DEBUG_DEATH((void)src2.value(), "");
}

// NOLINTEND(readability-magic-numbers)
