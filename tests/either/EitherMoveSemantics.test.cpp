// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include "TestHelpers.hpp"

TEST(EitherMoveSemantics, UNIT_006_MoveConstruct)
{
  RecordProperty("id", "0.01-UNIT-006");
  RecordProperty("desc", "Move constructor transfers ownership correctly");

  Either<int, std::string> srcErr{std::string("error")};
  EXPECT_TRUE(srcErr.error());
  Either<int, std::string> dstErr{std::move(srcErr)};
  EXPECT_FALSE(srcErr.error());
  ASSERT_TRUE(dstErr.error());
  EXPECT_EQ(*dstErr.error(), "error");

  Either<int, std::string> srcData{42};
  EXPECT_TRUE(srcData.data());
  Either<int, std::string> dstData{std::move(srcData)};
  ASSERT_TRUE(dstData.data());
  EXPECT_EQ(*dstData.data(), 42);
  EXPECT_FALSE(srcErr.data());
}

TEST(EitherMoveSemantics, UNIT_007_MoveAssign)
{
  RecordProperty("id", "0.01-UNIT-007");
  RecordProperty("desc", "Move assignment overwrites data with error and vice versa");

  Either<int, std::string> src1{std::string("new error")};
  Either<int, std::string> dst1{100};
  dst1 = std::move(src1);
  EXPECT_FALSE(src1.error());
  EXPECT_FALSE(src1.data());
  EXPECT_FALSE(dst1.data());
  ASSERT_TRUE(dst1.error());
  EXPECT_EQ(*dst1.error(), "new error");

  Either<int, std::string> src2{200};
  Either<int, std::string> dst2{std::string("old error")};
  dst2 = std::move(src2);
  EXPECT_FALSE(src2.error());
  EXPECT_FALSE(src2.data());
  EXPECT_FALSE(dst2.error());
  ASSERT_TRUE(dst2.data());
  EXPECT_EQ(*dst2.data(), 200);
}

TEST(EitherMoveSemantics, UNIT_008_SelfMoveAssign)
{
  RecordProperty("id", "0.01-UNIT-008");
  RecordProperty("desc", "Self-move-assignment does not crash");

  Either<int, std::string> e{42};
  // Intentionally testing self-move-assignment behavior
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#endif
  e = std::move(e);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  SUCCEED();
}

TEST(EitherMoveSemantics, UNIT_009_MoveFromLvalue)
{
  RecordProperty("id", "0.01-UNIT-009");
  RecordProperty("desc", "Move construct from lvalue using explicit std::move");

  Either<int, std::string> &&src{42};
  Either<int, std::string> dst{std::move(src)};
  ASSERT_TRUE(dst.data());
  EXPECT_EQ(*dst.data(), 42);
}

TEST(EitherMoveSemantics, UNIT_010_ZeroCopies)
{
  RecordProperty("id", "0.01-UNIT-010");
  RecordProperty("desc", "Move operations use move semantics, zero copies");

  MoveTracker::reset();
  Either<MoveTracker, std::string> src{MoveTracker{42}};
  Either<MoveTracker, std::string> dst{std::move(src)};
  EXPECT_EQ(MoveTracker::copyCount, 0);
  EXPECT_GT(MoveTracker::moveCount, 0);
  ASSERT_TRUE(dst.data());
  EXPECT_EQ(dst.data()->value, 42);

  MoveTracker::reset();
  Either<MoveTracker, std::string> src2{MoveTracker{42}};
  Either<MoveTracker, std::string> dst2{MoveTracker{0}};
  dst2 = std::move(src2);
  EXPECT_EQ(MoveTracker::copyCount, 0);
  EXPECT_GT(MoveTracker::moveCount, 0);

  MoveTracker::reset();
  Either<int, MoveTracker> errSrc{MoveTracker{-1}};
  Either<int, MoveTracker> errDst{std::move(errSrc)};
  EXPECT_EQ(MoveTracker::copyCount, 0);
  ASSERT_TRUE(errDst.error());
  EXPECT_EQ(errDst.error()->value, -1);
}
