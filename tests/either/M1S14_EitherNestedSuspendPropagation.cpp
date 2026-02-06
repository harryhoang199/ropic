// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)

namespace
{

constexpr bool kReturnError = true;
constexpr bool kReturnOk = false;

auto layer3(ManualResumeAwaiter* suspendL3, bool returnError)
    -> Either<Void, std::string>
{
  if (suspendL3 != nullptr)
    co_await *suspendL3;
  if (returnError)
    co_return std::string("nested suspend error");
  co_return OK;
}

auto layer2(
    ManualResumeAwaiter* suspendL2,
    ManualResumeAwaiter* suspendL3,
    bool returnError) -> Either<Void, std::string>
{
  if (suspendL2 != nullptr)
    co_await *suspendL2;
  co_await layer3(suspendL3, returnError);
  co_return OK;
}

auto layer1(
    ManualResumeAwaiter* suspendL1,
    ManualResumeAwaiter* suspendL2,
    ManualResumeAwaiter* suspendL3,
    bool returnError) -> Either<Void, std::string>
{
  if (suspendL1 != nullptr)
    co_await *suspendL1;
  co_await layer2(suspendL2, suspendL3, returnError);
  co_return OK;
}

auto outermost(
    ManualResumeAwaiter* suspendL1,
    ManualResumeAwaiter* suspendL2,
    ManualResumeAwaiter* suspendL3,
    bool returnError) -> Either<Void, std::string>
{
  co_await layer1(suspendL1, suspendL2, suspendL3, returnError);
  co_return OK;
}

} // namespace

// =============================================================================
// Test Suite: Nested Suspend Error/Success Propagation
// =============================================================================
// Tests error and success propagation through a 4-level nested
// Either<Void, std::string> coroutine chain (outermost -> layer1 -> layer2 ->
// layer3) with ManualResumeAwaiter suspension at various layers.
// Each layer receives pointers to its own awaiter (nullptr = no suspension).

// ---------------------------------------------------------------------------
// Group A — Error Propagation (layer3 co_return ERROR)
// ---------------------------------------------------------------------------

TEST(M1S14_EitherNestedSuspendPropagation, U01_SyncChainPropagatesError)
{
  RecordProperty("id", "M1-S14-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "Error propagates through 4-level chain with no suspension");

  auto result = outermost(nullptr, nullptr, nullptr, kReturnError);

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(*result.error(), "nested suspend error");
}

TEST(M1S14_EitherNestedSuspendPropagation, U02_Layer1SuspendedPropagatesError)
{
  RecordProperty("id", "M1-S14-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Error propagates through 4-level chain with layer1 "
      "suspended");

  ManualResumeAwaiter awaiterLayer1;

  auto result = outermost(&awaiterLayer1, nullptr, nullptr, kReturnError);

  EXPECT_FALSE(result.done());
  awaiterLayer1.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(*result.error(), "nested suspend error");
}

TEST(M1S14_EitherNestedSuspendPropagation, U03_Layer2SuspendedPropagatesError)
{
  RecordProperty("id", "M1-S14-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Error propagates through 4-level chain with layer2 "
      "suspended");

  ManualResumeAwaiter awaiterLayer2;

  auto result = outermost(nullptr, &awaiterLayer2, nullptr, kReturnError);

  EXPECT_FALSE(result.done());
  awaiterLayer2.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(*result.error(), "nested suspend error");
}

TEST(M1S14_EitherNestedSuspendPropagation, U04_Layer3SuspendedReturnsError)
{
  RecordProperty("id", "M1-S14-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Error propagates through 4-level chain with layer3 "
      "suspended");

  ManualResumeAwaiter awaiterLayer3;

  auto result = outermost(nullptr, nullptr, &awaiterLayer3, kReturnError);

  EXPECT_FALSE(result.done());
  awaiterLayer3.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(*result.error(), "nested suspend error");
}

TEST(
    M1S14_EitherNestedSuspendPropagation, U05_Layer1And2SuspendedPropagateError)
{
  RecordProperty("id", "M1-S14-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Error propagates through 4-level chain with layer1 and "
      "layer2 suspended");

  ManualResumeAwaiter awaiterLayer1;
  ManualResumeAwaiter awaiterLayer2;

  auto result =
      outermost(&awaiterLayer1, &awaiterLayer2, nullptr, kReturnError);

  EXPECT_FALSE(result.done());
  awaiterLayer1.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer2.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(*result.error(), "nested suspend error");
}

TEST(
    M1S14_EitherNestedSuspendPropagation, U06_Layer1And3SuspendedPropagateError)
{
  RecordProperty("id", "M1-S14-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Error propagates through 4-level chain with layer1 and "
      "layer3 suspended");

  ManualResumeAwaiter awaiterLayer1;
  ManualResumeAwaiter awaiterLayer3;

  auto result =
      outermost(&awaiterLayer1, nullptr, &awaiterLayer3, kReturnError);

  EXPECT_FALSE(result.done());
  awaiterLayer1.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer3.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(*result.error(), "nested suspend error");
}

TEST(
    M1S14_EitherNestedSuspendPropagation, U07_Layer2And3SuspendedPropagateError)
{
  RecordProperty("id", "M1-S14-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Error propagates through 4-level chain with layer2 and "
      "layer3 suspended");

  ManualResumeAwaiter awaiterLayer2;
  ManualResumeAwaiter awaiterLayer3;

  auto result =
      outermost(nullptr, &awaiterLayer2, &awaiterLayer3, kReturnError);

  EXPECT_FALSE(result.done());
  awaiterLayer2.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer3.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(*result.error(), "nested suspend error");
}

TEST(M1S14_EitherNestedSuspendPropagation, U08_AllLayersSuspendedPropagateError)
{
  RecordProperty("id", "M1-S14-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Error propagates through 4-level chain with layer1, layer2, "
      "and layer3 all suspended");

  ManualResumeAwaiter awaiterLayer1;
  ManualResumeAwaiter awaiterLayer2;
  ManualResumeAwaiter awaiterLayer3;

  auto result =
      outermost(&awaiterLayer1, &awaiterLayer2, &awaiterLayer3, kReturnError);

  EXPECT_FALSE(result.done());
  awaiterLayer1.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer2.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer3.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(*result.error(), "nested suspend error");
}

// ---------------------------------------------------------------------------
// Group B — Success Propagation (layer3 co_return OK)
// ---------------------------------------------------------------------------

TEST(M1S14_EitherNestedSuspendPropagation, U09_SyncChainPropagatesOk)
{
  RecordProperty("id", "M1-S14-U09");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc", "Success propagates through 4-level chain with no suspension");

  auto result = outermost(nullptr, nullptr, nullptr, kReturnOk);

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
}

TEST(M1S14_EitherNestedSuspendPropagation, U10_Layer1SuspendedPropagatesOk)
{
  RecordProperty("id", "M1-S14-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Success propagates through 4-level chain with layer1 "
      "suspended");

  ManualResumeAwaiter awaiterLayer1;

  auto result = outermost(&awaiterLayer1, nullptr, nullptr, kReturnOk);

  EXPECT_FALSE(result.done());
  awaiterLayer1.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
}

TEST(M1S14_EitherNestedSuspendPropagation, U11_Layer2SuspendedPropagatesOk)
{
  RecordProperty("id", "M1-S14-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Success propagates through 4-level chain with layer2 "
      "suspended");

  ManualResumeAwaiter awaiterLayer2;

  auto result = outermost(nullptr, &awaiterLayer2, nullptr, kReturnOk);

  EXPECT_FALSE(result.done());
  awaiterLayer2.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
}

TEST(M1S14_EitherNestedSuspendPropagation, U12_Layer3SuspendedReturnsOk)
{
  RecordProperty("id", "M1-S14-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Success propagates through 4-level chain with layer3 "
      "suspended");

  ManualResumeAwaiter awaiterLayer3;

  auto result = outermost(nullptr, nullptr, &awaiterLayer3, kReturnOk);

  EXPECT_FALSE(result.done());
  awaiterLayer3.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
}

TEST(M1S14_EitherNestedSuspendPropagation, U13_Layer1And2SuspendedPropagateOk)
{
  RecordProperty("id", "M1-S14-U13");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Success propagates through 4-level chain with layer1 and "
      "layer2 suspended");

  ManualResumeAwaiter awaiterLayer1;
  ManualResumeAwaiter awaiterLayer2;

  auto result = outermost(&awaiterLayer1, &awaiterLayer2, nullptr, kReturnOk);

  EXPECT_FALSE(result.done());
  awaiterLayer1.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer2.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
}

TEST(M1S14_EitherNestedSuspendPropagation, U14_Layer1And3SuspendedPropagateOk)
{
  RecordProperty("id", "M1-S14-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Success propagates through 4-level chain with layer1 and "
      "layer3 suspended");

  ManualResumeAwaiter awaiterLayer1;
  ManualResumeAwaiter awaiterLayer3;

  auto result = outermost(&awaiterLayer1, nullptr, &awaiterLayer3, kReturnOk);

  EXPECT_FALSE(result.done());
  awaiterLayer1.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer3.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
}

TEST(M1S14_EitherNestedSuspendPropagation, U15_Layer2And3SuspendedPropagateOk)
{
  RecordProperty("id", "M1-S14-U15");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Success propagates through 4-level chain with layer2 and "
      "layer3 suspended");

  ManualResumeAwaiter awaiterLayer2;
  ManualResumeAwaiter awaiterLayer3;

  auto result = outermost(nullptr, &awaiterLayer2, &awaiterLayer3, kReturnOk);

  EXPECT_FALSE(result.done());
  awaiterLayer2.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer3.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
}

TEST(M1S14_EitherNestedSuspendPropagation, U16_AllLayersSuspendedPropagateOk)
{
  RecordProperty("id", "M1-S14-U16");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Success propagates through 4-level chain with layer1, "
      "layer2, and layer3 all suspended");

  ManualResumeAwaiter awaiterLayer1;
  ManualResumeAwaiter awaiterLayer2;
  ManualResumeAwaiter awaiterLayer3;

  auto result =
      outermost(&awaiterLayer1, &awaiterLayer2, &awaiterLayer3, kReturnOk);

  EXPECT_FALSE(result.done());
  awaiterLayer1.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer2.resume();

  EXPECT_FALSE(result.done());
  awaiterLayer3.resume();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
