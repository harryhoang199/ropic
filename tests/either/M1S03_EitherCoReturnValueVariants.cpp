// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "../shared/FixedString.hpp"
#include "ropic.hpp"
using namespace ropic;

// NOLINTBEGIN(readability-magic-numbers)

// =============================================================================
// Local Test Types (only used in this file)
// =============================================================================

/// @brief Multi-argument constructible VALUE type for tuple return tests.
struct Coordinate
{
  int x;
  int y;
  std::string label;

  Coordinate(int xVal, int yVal, std::string lbl)
      : x(xVal),
        y(yVal),
        label(std::move(lbl))
  {
  }

  auto operator==(const Coordinate& other) const -> bool = default;
};

/// @brief Tracks destructor calls for lifetime verification tests (VALUE type).
/// Template parameter ID ensures per-test isolation for parallel execution.
template <FixedString ID>
struct DestructorTracker
{
  static int s_destructorCount;
  int id;

  explicit DestructorTracker(int idVal)
      : id(idVal)
  {
  }
  ~DestructorTracker() { ++s_destructorCount; }

  DestructorTracker(const DestructorTracker&) = delete;
  auto operator=(const DestructorTracker&) -> DestructorTracker& = delete;

  DestructorTracker(DestructorTracker&& other) noexcept
      : id(other.id)
  {
    other.id = -1;
  }

  auto operator=(DestructorTracker&&) -> DestructorTracker& = delete;

  static void reset() { s_destructorCount = 0; }
};

template <FixedString ID>
int DestructorTracker<ID>::s_destructorCount = 0;

/// @brief VALUE type with deleted move constructor for in-place construction
/// tests.
struct ImmovableValue
{
  int x;
  int y;
  std::string label;

  ImmovableValue(int xVal, int yVal, std::string lbl)
      : x(xVal),
        y(yVal),
        label(std::move(lbl))
  {
  }

  ImmovableValue(const ImmovableValue&) = delete;
  auto operator=(const ImmovableValue&) -> ImmovableValue& = delete;
  ImmovableValue(ImmovableValue&&) = delete;
  auto operator=(ImmovableValue&&) -> ImmovableValue& = delete;

  auto operator==(const ImmovableValue& other) const -> bool = default;
};

TEST(M1S03_EitherCoReturnValueVariants, U01_BraceInitCoreturnValue)
{
  RecordProperty("id", "M1-S03-U01");
  RecordProperty("ver", "0.03");
  RecordProperty("desc", "co_return {args...} constructs VALUE via brace-init");

  auto returnCoordinateBraceInit =
      [](int x, int y, std::string label) -> Either<Coordinate, std::string>
  { co_return {x, y, std::move(label)}; };

  auto result = returnCoordinateBraceInit(10, 20, "origin");

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
  EXPECT_EQ(result.value()->x, 10);
  EXPECT_EQ(result.value()->y, 20);
  EXPECT_EQ(result.value()->label, "origin");
}

TEST(M1S03_EitherCoReturnValueVariants, U02_DirectCoreturnValueEquivalent)
{
  RecordProperty("id", "M1-S03-U02");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "Direct co_return Type{args...} produces same result as brace-init");

  auto returnCoordinateDirect =
      [](int x, int y, std::string label) -> Either<Coordinate, std::string>
  { co_return Coordinate{x, y, std::move(label)}; };

  auto returnCoordinateBraceInit =
      [](int x, int y, std::string label) -> Either<Coordinate, std::string>
  { co_return {x, y, std::move(label)}; };

  auto directResult = returnCoordinateDirect(10, 20, "origin");
  auto braceInitResult = returnCoordinateBraceInit(10, 20, "origin");

  ASSERT_TRUE(directResult.done());
  ASSERT_TRUE(braceInitResult.done());
  ASSERT_TRUE(directResult.value());
  ASSERT_TRUE(braceInitResult.value());
  ASSERT_FALSE(directResult.error());
  ASSERT_FALSE(braceInitResult.error());

  EXPECT_EQ(*directResult.value(), *braceInitResult.value());
}

TEST(M1S03_EitherCoReturnValueVariants, U03_ConstRefCoreturnValue)
{
  RecordProperty("id", "M1-S03-U03");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return const VALUE& invokes return_value(VALUE const&) overload");

  const Coordinate coord{50, 60, "reference"};
  auto returnConstCoordinate = [&coord]() -> Either<Coordinate, std::string>
  { co_return coord; };

  auto result = returnConstCoordinate();

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
  EXPECT_EQ(result.value()->x, coord.x);
  EXPECT_EQ(result.value()->y, coord.y);
  EXPECT_EQ(result.value()->label, coord.label);
}

TEST(M1S03_EitherCoReturnValueVariants, U04_TupleImmovableValue)
{
  RecordProperty("id", "M1-S03-U04");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return std::make_tuple constructs immovable VALUE in-place without "
      "move");

  auto returnImmovableExplicitTuple =
      [](int x, int y, std::string label) -> Either<ImmovableValue, std::string>
  { co_return std::make_tuple(x, y, std::move(label)); };

  auto result = returnImmovableExplicitTuple(30, 40, "vector");

  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.value());
  ASSERT_FALSE(result.error());
  EXPECT_EQ(result.value()->x, 30);
  EXPECT_EQ(result.value()->y, 40);
  EXPECT_EQ(result.value()->label, "vector");
}

TEST(M1S03_EitherCoReturnValueVariants, U05_ValueDestructorOnEitherDestroy)
{
  RecordProperty("id", "M1-S03-U05");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "Destructor of VALUE is called when Either is destroyed");

  using DT = DestructorTracker<"M1-S03-U05">;

  auto returnDestructorTracker = [](int id) -> Either<DT, std::string>
  { co_return DT{id}; };

  DT::reset();
  ASSERT_EQ(DT::s_destructorCount, 0);

  {
    auto result = returnDestructorTracker(42);
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.value());
    ASSERT_FALSE(result.error());
    EXPECT_EQ(result.value()->id, 42);
    // Destructor not yet called (Either still in scope)
  }

  // After Either goes out of scope, destructor should have been called
  // Note: Move construction in coroutine may cause additional destructor calls
  EXPECT_GE(DT::s_destructorCount, 1);
}

TEST(M1S03_EitherCoReturnValueVariants, U06_NoValueDestructorOnError)
{
  RecordProperty("id", "M1-S03-U06");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "VALUE destructor is NOT called when Either contains error");

  using DT = DestructorTracker<"M1-S03-U06">;

  auto returnDestructorTrackerError =
      [](std::string msg) -> Either<DT, std::string> { co_return msg; };

  DT::reset();
  ASSERT_EQ(DT::s_destructorCount, 0);

  {
    auto result = returnDestructorTrackerError("some error");
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    ASSERT_FALSE(result.value());
    EXPECT_EQ(*result.error(), "some error");
  }

  // No DestructorTracker was ever constructed, so count should be 0
  EXPECT_EQ(DT::s_destructorCount, 0);
}

TEST(M1S03_EitherCoReturnValueVariants, U07_MultipleDestructorCalls)
{
  RecordProperty("id", "M1-S03-U07");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc", "Multiple Either instances track destructor calls independently");

  using DT = DestructorTracker<"M1-S03-U07">;

  auto returnDestructorTracker = [](int id) -> Either<DT, std::string>
  { co_return DT{id}; };

  DT::reset();
  ASSERT_EQ(DT::s_destructorCount, 0);

  int countAfterFirst = 0;
  {
    auto result1 = returnDestructorTracker(1);
    ASSERT_TRUE(result1.value());
    ASSERT_FALSE(result1.error());
  }
  countAfterFirst = DT::s_destructorCount;
  EXPECT_GE(countAfterFirst, 1);

  {
    auto result2 = returnDestructorTracker(2);
    ASSERT_TRUE(result2.value());
    ASSERT_FALSE(result2.error());
  }
  EXPECT_GT(DT::s_destructorCount, countAfterFirst);

  {
    auto result3 = returnDestructorTracker(3);
    ASSERT_TRUE(result3.value());
    ASSERT_FALSE(result3.error());
  }
  EXPECT_GT(
      DT::s_destructorCount,
      countAfterFirst + ((DT::s_destructorCount - countAfterFirst) / 2));
}

// NOLINTEND(readability-magic-numbers)
