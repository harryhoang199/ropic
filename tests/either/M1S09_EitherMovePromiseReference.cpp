// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>
#include <mutex>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)

// =============================================================================
// Test Suite: Move Semantics with Promise Reference Verification
//
// =============================================================================

TEST(M1S09EitherMovePromiseReference, U01MoveWhileSuspendedThenResume)
{
  RecordProperty("id", "M1-S09-U01");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Move Either while suspended, then resume - promise correctly references "
      "new Either");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 42;
  };

  auto originalEither = coro();

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);

  // Move to new location
  auto movedEither = std::move(originalEither);

  // Verify original is moved-from (empty state)
  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED)
      << "Moved-from Either should report UNDEFINED";

  // Verify moved-to is still not done (suspended)
  EXPECT_EQ(movedEither.state(), ropic::CoroState::UNDEFINED)
      << "Moved Either should still be UNDEFINED while suspended";

  // Resume - this should store result in movedEither, not originalEither
  awaiter.resume();

  // Verify moved-to Either received the result
  EXPECT_EQ(movedEither.state(), ropic::CoroState::DONE)
      << "Moved Either should be done after resume ";
  EXPECT_TRUE(movedEither.value())
      << "Moved Either should have data after resume";
  EXPECT_EQ(*movedEither.value(), 42);

  // Verify original is still empty
  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);
}

TEST(M1S09EitherMovePromiseReference, U02MoveAssignWhileSuspendedThenResume)
{
  RecordProperty("id", "M1-S09-U02");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Move-assign Either while suspended, then resume - promise correctly "
      "references new Either");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 99;
  };

  auto originalEither = coro();
  auto targetEither = []() -> Either<int, std::string> { co_return 0; }();

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);
  EXPECT_EQ(targetEither.state(), ropic::CoroState::DONE);

  // Move-assign to existing Either
  targetEither = std::move(originalEither);

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);
  EXPECT_EQ(targetEither.state(), ropic::CoroState::UNDEFINED);

  // Resume
  awaiter.resume();

  EXPECT_EQ(targetEither.state(), ropic::CoroState::DONE);
  EXPECT_TRUE(targetEither.value());
  EXPECT_EQ(*targetEither.value(), 99);

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);
}

TEST(M1S09EitherMovePromiseReference, U03MoveWhileSuspendedErrorPath)
{
  RecordProperty("id", "M1-S09-U03");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Move Either while suspended, resume with error - promise correctly "
      "references new Either");

  ManualResumeAwaiterWithValue<bool> awaiter{true};

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    bool shouldError = co_await awaiter;
    if (shouldError)
      co_return std::string("error after move");
    co_return 123;
  };

  auto originalEither = coro();

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);

  auto movedEither = std::move(originalEither);

  EXPECT_EQ(movedEither.state(), ropic::CoroState::UNDEFINED);

  awaiter.resume();

  EXPECT_EQ(movedEither.state(), ropic::CoroState::DONE);
  EXPECT_TRUE(movedEither.error());
  EXPECT_EQ(*movedEither.error(), "error after move");

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);
}

TEST(M1S09EitherMovePromiseReference, U04MultipleMovesThenResume)
{
  RecordProperty("id", "M1-S09-U04");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Move Either multiple times while suspended - promise tracks final "
      "location");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<int, std::string>
  {
    co_await awaiter;
    co_return 777;
  };

  auto either1 = coro();
  EXPECT_EQ(either1.state(), ropic::CoroState::UNDEFINED);

  auto either2 = std::move(either1);
  EXPECT_EQ(either1.state(), ropic::CoroState::UNDEFINED);
  EXPECT_EQ(either2.state(), ropic::CoroState::UNDEFINED);

  auto either3 = std::move(either2);
  EXPECT_EQ(either2.state(), ropic::CoroState::UNDEFINED);
  EXPECT_EQ(either3.state(), ropic::CoroState::UNDEFINED);

  auto either4 = []() -> Either<int, std::string> { co_return 0; }();
  either4 = std::move(either3);
  EXPECT_EQ(either3.state(), ropic::CoroState::UNDEFINED);
  EXPECT_EQ(either4.state(), ropic::CoroState::UNDEFINED);

  awaiter.resume();

  // Only the final location should have the result
  ASSERT_EQ(either4.state(), ropic::CoroState::DONE);
  EXPECT_TRUE(either4.value());
  EXPECT_EQ(*either4.value(), 777);

  // All others should be empty
  EXPECT_EQ(either3.state(), ropic::CoroState::UNDEFINED);
}

TEST(M1S09EitherMovePromiseReference, U05MoveVoidEitherWhileSuspended)
{
  RecordProperty("id", "M1-S09-U05");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Move Either<Void, Error> while suspended - promise correctly references "
      "new Either");

  ManualResumeAwaiter awaiter;

  auto coro = [&awaiter]() -> Either<Void, std::string>
  {
    co_await awaiter;
    co_return OK;
  };

  auto originalEither = coro();

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);

  auto movedEither = std::move(originalEither);

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);
  EXPECT_EQ(movedEither.state(), ropic::CoroState::UNDEFINED);

  awaiter.resume();

  EXPECT_EQ(movedEither.state(), ropic::CoroState::DONE);
  EXPECT_FALSE(movedEither.error());

  EXPECT_EQ(originalEither.state(), ropic::CoroState::UNDEFINED);
}

TEST(M1S09EitherMovePromiseReference, U06AsyncMoveWhileSuspended)
{
  RecordProperty("id", "M1-S09-U06");
  RecordProperty("ver", "0.02");
  RecordProperty(
      "desc",
      "Move Either during async operation - promise correctly references new "
      "Either");

  auto mutex = std::make_shared<std::mutex>();

  auto coro = [mutex]() -> Either<int, std::string>
  {
    co_await AsyncDelayAwaiter{std::chrono::milliseconds(30), mutex};
    co_return 555;
  };

  Either<int, std::string>* originalEitherPtr;
  Either<int, std::string>* movedEitherPtr;

  {
    std::lock_guard lock(*mutex);
    originalEitherPtr = new Either<int, std::string>(coro());
    EXPECT_EQ(originalEitherPtr->state(), ropic::CoroState::UNDEFINED);

    // Move immediately (while async operation in flight)
    movedEitherPtr =
        new Either<int, std::string>(std::move(*originalEitherPtr));

    EXPECT_EQ(originalEitherPtr->state(), ropic::CoroState::UNDEFINED);
    EXPECT_EQ(originalEitherPtr->state(), ropic::CoroState::UNDEFINED);
  }

  // Wait for async completion
  ASSERT_TRUE(pollUntilDone(
      [movedEitherPtr, mutex]
      {
        std::lock_guard lock(*mutex);
        return movedEitherPtr->state() == ropic::CoroState::DONE;
      },
      std::chrono::seconds(2)))
      << "Timeout waiting for async completion";

  EXPECT_EQ(movedEitherPtr->state(), ropic::CoroState::DONE);
  EXPECT_TRUE(movedEitherPtr->value());
  EXPECT_EQ(*(movedEitherPtr->value()), 555);

  EXPECT_EQ(originalEitherPtr->state(), ropic::CoroState::UNDEFINED);

  delete originalEitherPtr;
  delete movedEitherPtr;
}

// NOLINTEND(readability-magic-numbers,readability-identifier-naming,readability-convert-member-functions-to-static)
