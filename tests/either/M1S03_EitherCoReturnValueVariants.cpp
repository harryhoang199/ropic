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

namespace
{

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

/// @brief Tracks destructor calls for lifetime verification tests
/// (VALUE type). Template parameter ID ensures per-test isolation for
/// parallel execution.
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

/// @brief VALUE type with deleted move constructor for in-place
/// construction tests.
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

} // namespace

TEST(M1S03EitherCoReturnValueVariants, U01BraceInitCoreturnValue)
{
  RecordProperty("id", "M1-S03-U01");
  RecordProperty("ver", "0.03");
  RecordProperty("desc", "co_return {args...} constructs VALUE via brace-init");

  auto returnCoordinateBraceInit =
      [](int x, int y, std::string label) -> Either<Coordinate, std::string>
  { co_return {x, y, std::move(label)}; };

  auto result = returnCoordinateBraceInit(10, 20, "origin");

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(result.value()->x, 10);
  EXPECT_EQ(result.value()->y, 20);
  EXPECT_EQ(result.value()->label, "origin");
}

TEST(M1S03EitherCoReturnValueVariants, U02DirectCoreturnValueEquivalent)
{
  RecordProperty("id", "M1-S03-U02");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "Direct co_return Type{args...} produces same result as "
      "brace-init");

  auto returnCoordinateDirect =
      [](int x, int y, std::string label) -> Either<Coordinate, std::string>
  { co_return Coordinate{x, y, std::move(label)}; };

  auto returnCoordinateBraceInit =
      [](int x, int y, std::string label) -> Either<Coordinate, std::string>
  { co_return {x, y, std::move(label)}; };

  auto directResult = returnCoordinateDirect(10, 20, "origin");
  auto braceInitResult = returnCoordinateBraceInit(10, 20, "origin");

  ASSERT_EQ(directResult.state(), ropic::CoroState::DONE);
  ASSERT_EQ(braceInitResult.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(directResult.value());
  ASSERT_TRUE(braceInitResult.value());

  EXPECT_EQ(*directResult.value(), *braceInitResult.value());
}

TEST(M1S03EitherCoReturnValueVariants, U03ConstRefCoreturnValue)
{
  RecordProperty("id", "M1-S03-U03");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return const VALUE& invokes return_value(VALUE const&) "
      "overload");

  const Coordinate coord{50, 60, "reference"};
  auto returnConstCoordinate = [&coord]() -> Either<Coordinate, std::string>
  { co_return coord; };

  auto result = returnConstCoordinate();

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(result.value()->x, coord.x);
  EXPECT_EQ(result.value()->y, coord.y);
  EXPECT_EQ(result.value()->label, coord.label);
}

TEST(M1S03EitherCoReturnValueVariants, U04TupleImmovableValue)
{
  RecordProperty("id", "M1-S03-U04");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "co_return std::make_tuple constructs immovable VALUE in-place "
      "without move");

  auto returnImmovableExplicitTuple =
      [](int x, int y, std::string label) -> Either<ImmovableValue, std::string>
  { co_return std::make_tuple(x, y, std::move(label)); };

  auto result = returnImmovableExplicitTuple(30, 40, "vector");

  ASSERT_EQ(result.state(), ropic::CoroState::DONE);
  ASSERT_TRUE(result.value());
  EXPECT_EQ(result.value()->x, 30);
  EXPECT_EQ(result.value()->y, 40);
  EXPECT_EQ(result.value()->label, "vector");
}

TEST(M1S03EitherCoReturnValueVariants, U05ValueDestructorOnEitherDestroy)
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
    ASSERT_EQ(result.state(), ropic::CoroState::DONE);
    ASSERT_TRUE(result.value());
    EXPECT_EQ(result.value()->id, 42);
  }

  EXPECT_GE(DT::s_destructorCount, 1);
}

TEST(M1S03EitherCoReturnValueVariants, U06NoValueDestructorOnError)
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
    ASSERT_EQ(result.state(), ropic::CoroState::DONE);
    ASSERT_TRUE(result.error());
    EXPECT_EQ(*result.error(), "some error");
  }

  EXPECT_EQ(DT::s_destructorCount, 0);
}

TEST(M1S03EitherCoReturnValueVariants, U07MultipleDestructorCalls)
{
  RecordProperty("id", "M1-S03-U07");
  RecordProperty("ver", "0.03");
  RecordProperty(
      "desc",
      "Multiple Either instances track destructor calls "
      "independently");

  using DT = DestructorTracker<"M1-S03-U07">;

  auto returnDestructorTracker = [](int id) -> Either<DT, std::string>
  { co_return DT{id}; };

  DT::reset();
  ASSERT_EQ(DT::s_destructorCount, 0);

  int countAfterFirst = 0;
  {
    auto result1 = returnDestructorTracker(1);
    ASSERT_TRUE(result1.value());
  }
  countAfterFirst = DT::s_destructorCount;
  EXPECT_GE(countAfterFirst, 1);

  {
    auto result2 = returnDestructorTracker(2);
    ASSERT_TRUE(result2.value());
  }
  EXPECT_GT(DT::s_destructorCount, countAfterFirst);

  {
    auto result3 = returnDestructorTracker(3);
    ASSERT_TRUE(result3.value());
  }
  EXPECT_GE(DT::s_destructorCount, 3);
}

// NOLINTEND(readability-magic-numbers)
