// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)
TEST(M1S02_EitherMoveSemantics, U01_MoveConstruct)
{
  RecordProperty("id", "M1-S02-U01");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Move constructor transfers ownership correctly");

  Either<int, std::string> srcErr = returnError("error");
  ASSERT_TRUE(srcErr.done());
  EXPECT_TRUE(srcErr.error());
  Either<int, std::string> dstErr{std::move(srcErr)};
  EXPECT_FALSE(srcErr.done());
  EXPECT_FALSE(srcErr.error());
  ASSERT_TRUE(dstErr.done());
  ASSERT_TRUE(dstErr.error());
  EXPECT_EQ(*dstErr.error(), "error");

  Either<int, std::string> srcData = returnData(42);
  ASSERT_TRUE(srcData.done());
  EXPECT_TRUE(srcData.value());
  Either<int, std::string> dstData{std::move(srcData)};
  EXPECT_FALSE(srcData.done());
  ASSERT_TRUE(dstData.done());
  ASSERT_TRUE(dstData.value());
  EXPECT_EQ(*dstData.value(), 42);
  EXPECT_FALSE(srcErr.value());
}

TEST(M1S02_EitherMoveSemantics, U02_MoveAssign)
{
  RecordProperty("id", "M1-S02-U02");
  RecordProperty("ver", "0.01");
  RecordProperty(
      "desc", "Move assignment overwrites data with error and vice versa");

  Either<int, std::string> src1 = returnError("new error");
  Either<int, std::string> dst1 = returnData(100);
  ASSERT_TRUE(src1.done());
  ASSERT_TRUE(dst1.done());
  dst1 = std::move(src1);
  EXPECT_FALSE(src1.done());
  EXPECT_FALSE(src1.error());
  EXPECT_FALSE(src1.value());
  ASSERT_TRUE(dst1.done());
  EXPECT_FALSE(dst1.value());
  ASSERT_TRUE(dst1.error());
  EXPECT_EQ(*dst1.error(), "new error");

  Either<int, std::string> src2 = returnData(200);
  Either<int, std::string> dst2 = returnError("old error");
  ASSERT_TRUE(src2.done());
  ASSERT_TRUE(dst2.done());
  dst2 = std::move(src2);
  EXPECT_FALSE(src2.done());
  EXPECT_FALSE(src2.error());
  EXPECT_FALSE(src2.value());
  ASSERT_TRUE(dst2.done());
  EXPECT_FALSE(dst2.error());
  ASSERT_TRUE(dst2.value());
  EXPECT_EQ(*dst2.value(), 200);
}

TEST(M1S02_EitherMoveSemantics, U03_SelfMoveAssign)
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
  SUCCEED();
}

TEST(M1S02_EitherMoveSemantics, U04_MoveFromRvalueRef)
{
  RecordProperty("id", "M1-S02-U04");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Move construct from rvalue reference");

  Either<int, std::string>&& src = returnData(42);
  Either<int, std::string> dst{std::move(src)};
  ASSERT_TRUE(dst.done());
  ASSERT_TRUE(dst.value());
  EXPECT_EQ(*dst.value(), 42);
}

TEST(M1S02_EitherMoveSemantics, U05_ZeroCopies)
{
  RecordProperty("id", "M1-S02-U05");
  RecordProperty("ver", "0.01");
  RecordProperty("desc", "Move operations use move semantics, zero copies");

  MoveTracker::reset();
  Either<MoveTracker, std::string> src = returnMoveTracker(42);
  ASSERT_TRUE(src.done());
  Either<MoveTracker, std::string> dst{std::move(src)};
  EXPECT_FALSE(src.done());
  EXPECT_EQ(MoveTracker::s_copyCount, 0);
  EXPECT_GT(MoveTracker::s_moveCount, 0);
  ASSERT_TRUE(dst.done());
  ASSERT_TRUE(dst.value());
  EXPECT_EQ(dst.value()->value, 42);

  MoveTracker::reset();
  Either<MoveTracker, std::string> src2 = returnMoveTracker(42);
  Either<MoveTracker, std::string> dst2 = returnMoveTracker(0);
  ASSERT_TRUE(src2.done());
  ASSERT_TRUE(dst2.done());
  dst2 = std::move(src2);
  EXPECT_FALSE(src2.done());
  ASSERT_TRUE(dst2.done());
  EXPECT_EQ(MoveTracker::s_copyCount, 0);
  EXPECT_GT(MoveTracker::s_moveCount, 0);

  MoveTracker::reset();
  Either<int, MoveTracker> errSrc = returnIntWithMoveTrackerError(true);
  ASSERT_TRUE(errSrc.done());
  Either<int, MoveTracker> errDst{std::move(errSrc)};
  EXPECT_FALSE(errSrc.done());
  EXPECT_EQ(MoveTracker::s_copyCount, 0);
  ASSERT_TRUE(errDst.done());
  ASSERT_TRUE(errDst.error());
  EXPECT_EQ(errDst.error()->value, -1);
}
// NOLINTEND(readability-magic-numbers)
