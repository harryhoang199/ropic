// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include <gtest/gtest.h>

#include "TestHelpers.hpp"

// NOLINTBEGIN(readability-magic-numbers)

/// @brief Independent base error class for multiple inheritance tests.
struct AuditableError
{
  int severity;
  std::string context;

  AuditableError(int s, std::string ctx)
      : severity(s),
        context(std::move(ctx))
  {
  }
  virtual ~AuditableError() = default;

  AuditableError(const AuditableError&) = default;
  auto operator=(const AuditableError&) -> AuditableError& = default;
  AuditableError(AuditableError&&) = default;
  auto operator=(AuditableError&&) -> AuditableError& = default;

  [[nodiscard]]
  virtual auto audit() const -> std::string
  {
    return "[severity=" + std::to_string(severity) + "] " + context;
  }
};

/// @brief Error inheriting from both BaseError and AuditableError.
struct AuditableApiError : BaseError,
                           AuditableError
{
  std::string service;

  AuditableApiError(
      int c, std::string msg, int sev, std::string ctx, std::string svc)
      : BaseError(c, std::move(msg)),
        AuditableError(sev, std::move(ctx)),
        service(std::move(svc))
  {
  }

  [[nodiscard]]
  auto describe() const -> std::string override
  {
    return message + " [" + service + "]";
  }

  [[nodiscard]]
  auto audit() const -> std::string override
  {
    return "[severity="
         + std::to_string(severity)
         + "] "
         + context
         + " ["
         + service
         + "]";
  }
};

/// @brief Tracks destructor calls for a convertible (non-derived) error type.
/// Convertible to ErrorDestructorTracker<ID> but NOT derived from it.
template <FixedString ID>
struct ConvertibleDestructorTracker
{
  static int s_destructorCount;
  int code;
  std::string message;

  ConvertibleDestructorTracker(int c, std::string msg)
      : code(c),
        message(std::move(msg))
  {
  }

  ~ConvertibleDestructorTracker() { ++s_destructorCount; }

  ConvertibleDestructorTracker(const ConvertibleDestructorTracker&) = delete;
  auto operator=(const ConvertibleDestructorTracker&)
      -> ConvertibleDestructorTracker& = delete;

  ConvertibleDestructorTracker(ConvertibleDestructorTracker&& other) noexcept
      : code(other.code),
        message(std::move(other.message))
  {
    other.code = -1;
  }

  auto operator=(ConvertibleDestructorTracker&&)
      -> ConvertibleDestructorTracker& = delete;

  operator ErrorDestructorTracker<ID>() const
  {
    return ErrorDestructorTracker<ID>{code, std::string(message)};
  }

  static void reset() { s_destructorCount = 0; }
};

template <FixedString ID>
int ConvertibleDestructorTracker<ID>::s_destructorCount = 0;

TEST(M1S13_EitherCrossTypeErrorPropagation, U01_DerivedError_TwoLevel)
{
  RecordProperty("id", "M1-S13-U01");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "NetworkError propagates from Either<int, NetworkError> to "
      "Either<double, BaseError>");

  auto returnNetworkError = [](int code, std::string msg, std::string endpoint)
      -> Either<int, NetworkError>
  { co_return NetworkError{code, std::move(msg), std::move(endpoint)}; };

  auto outer = [&returnNetworkError]() -> Either<double, BaseError>
  {
    int val = co_await returnNetworkError(500, "Connection failed", "/api/v1");
    co_return static_cast<double>(val);
  };

  auto result = outer();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 500);
  EXPECT_EQ(result.error()->message, "Connection failed");
  EXPECT_EQ(result.error()->describe(), "Connection failed at /api/v1");
}

TEST(M1S13_EitherCrossTypeErrorPropagation, U02_DerivedError_ThreeLevel_Nested)
{
  RecordProperty("id", "M1-S13-U02");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "ServiceError propagates through multi-level cross-type nesting via "
      "NetworkError to BaseError");

  auto level3 = []() -> Either<double, ServiceError>
  {
    co_return ServiceError{503, "Service Unavailable", "/api/deep", "auth-svc"};
  };

  auto level2 = [&level3]() -> Either<int, NetworkError>
  {
    double d = co_await level3();
    co_return static_cast<int>(d);
  };

  auto level1 = [&level2]() -> Either<std::string, BaseError>
  {
    int val = co_await level2();
    co_return std::to_string(val);
  };

  auto result = level1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 503);
  EXPECT_EQ(
      result.error()->describe(),
      "Service Unavailable at /api/deep [auth-svc]");
}

TEST(
    M1S13_EitherCrossTypeErrorPropagation,
    U03_DerivedError_ThreeLevel_SkipLevel)
{
  RecordProperty("id", "M1-S13-U03");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "ServiceError propagates directly from Either<double, ServiceError> to "
      "Either<string, BaseError> skipping intermediate NetworkError level");

  auto level3 = []() -> Either<double, ServiceError>
  {
    co_return ServiceError{503, "Service Unavailable", "/api/deep", "auth-svc"};
  };

  auto level1 = [&level3]() -> Either<std::string, BaseError>
  {
    double val = co_await level3();
    co_return std::to_string(static_cast<int>(val));
  };

  auto result = level1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 503);
  EXPECT_EQ(
      result.error()->describe(),
      "Service Unavailable at /api/deep [auth-svc]");
}

TEST(M1S13_EitherCrossTypeErrorPropagation, U04_DerivedError_Destructor)
{
  RecordProperty("id", "M1-S13-U04");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Derived error destructor called correctly after cross-type "
      "propagation");

  using BEDT = ErrorDestructorTracker<"M1-S13-U04">;
  using DEDT = DerivedErrorDestructorTracker<"M1-S13-U04">;

  auto returnDerivedError =
      [](int code, std::string msg, std::string detail) -> Either<int, DEDT>
  { co_return DEDT{code, std::move(msg), std::move(detail)}; };

  DEDT::resetDerived();

  {
    auto outer = [&returnDerivedError]() -> Either<int, BEDT>
    {
      int val = co_await returnDerivedError(404, "Not found", "/users");
      co_return val;
    };

    auto result = outer();
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    ASSERT_FALSE(result.value());
    EXPECT_EQ(result.error()->code, 404);
  }

  EXPECT_GE(DEDT::s_derivedDestructorCount, 1);
  EXPECT_GE(BEDT::s_destructorCount, 1);
}

TEST(M1S13_EitherCrossTypeErrorPropagation, DISABLED_U05_Convertible_TwoLevel)
{
  RecordProperty("id", "M1-S13-U06");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "LevelBError propagates from Either<Void, LevelBError> to "
      "Either<Void, LevelAError> via conversion operator (not derived)");

  auto inner = []() -> Either<Void, LevelBError>
  { co_return LevelBError{400, "Bad Request", "/api"}; };

  auto outer = [&inner]() -> Either<Void, LevelAError>
  {
    co_await inner();
    co_return OK;
  };

  auto result = outer();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 400);
  EXPECT_EQ(result.error()->message, "Bad Request");
}

TEST(M1S13_EitherCrossTypeErrorPropagation, DISABLED_U06_ConvChain_SkipLevel)
{
  RecordProperty("id", "M1-S13-U07");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "LevelCError propagates directly to Either<Void, LevelAError> "
      "(E3 conv E2, E2 conv E1, f1 co_awaits f3 with no intermediate)");

  auto f3 = []() -> Either<Void, LevelCError>
  { co_return LevelCError{422, "Validation", "/submit", "field:email"}; };

  auto f1 = [&f3]() -> Either<Void, LevelAError>
  {
    co_await f3();
    co_return OK;
  };

  auto result = f1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 422);
  EXPECT_EQ(result.error()->message, "Validation");
}

TEST(
    M1S13_EitherCrossTypeErrorPropagation,
    DISABLED_U07_DerivedThenConv_SkipLevel)
{
  RecordProperty("id", "M1-S13-U08");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "DerivedLevelBError propagates directly to Either<Void, LevelAError> "
      "(E3 derived from E2, E2 conv E1, f1 co_awaits f3 with no "
      "intermediate)");

  auto f3 = []() -> Either<Void, DerivedLevelBError>
  { co_return DerivedLevelBError{500, "Server Error", "/api", "trace-123"}; };

  auto f1 = [&f3]() -> Either<Void, LevelAError>
  {
    co_await f3();
    co_return OK;
  };

  auto result = f1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 500);
  EXPECT_EQ(result.error()->message, "Server Error");
}

TEST(
    M1S13_EitherCrossTypeErrorPropagation,
    DISABLED_U08_ConvThenDerived_SkipLevel)
{
  RecordProperty("id", "M1-S13-U09");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "ConvertibleToNetworkError propagates directly to "
      "Either<Void, BaseError> (E3 conv E2, E2 derived from E1, "
      "f1 co_awaits f3 with no intermediate)");

  auto f3 = []() -> Either<Void, ConvertibleToNetworkError>
  {
    co_return ConvertibleToNetworkError{
        502, "Bad Gateway", "/proxy", "upstream-timeout"};
  };

  auto f1 = [&f3]() -> Either<Void, BaseError>
  {
    co_await f3();
    co_return OK;
  };

  auto result = f1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 502);
  EXPECT_EQ(result.error()->message, "Bad Gateway");
}

TEST(M1S13_EitherCrossTypeErrorPropagation, DISABLED_U09_AllConv_Nested)
{
  RecordProperty("id", "M1-S13-U10");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "LevelCError propagates through f1->f2->f3 nested chain "
      "(E2 conv E1, E3 conv E2)");

  auto f3 = []() -> Either<Void, LevelCError>
  { co_return LevelCError{503, "Unavailable", "/svc", "retry-after:30"}; };

  auto f2 = [&f3]() -> Either<Void, LevelBError>
  {
    co_await f3();
    co_return OK;
  };

  auto f1 = [&f2]() -> Either<Void, LevelAError>
  {
    co_await f2();
    co_return OK;
  };

  auto result = f1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 503);
  EXPECT_EQ(result.error()->message, "Unavailable");
}

TEST(M1S13_EitherCrossTypeErrorPropagation, DISABLED_U10_ConvAndDerived_Nested)
{
  RecordProperty("id", "M1-S13-U11");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "DerivedLevelBError propagates through f1->f2->f3 nested chain "
      "(E2 conv E1, E3 derived from E2)");

  auto f3 = []() -> Either<Void, DerivedLevelBError>
  { co_return DerivedLevelBError{500, "Internal", "/api", "stack-trace"}; };

  auto f2 = [&f3]() -> Either<Void, LevelBError>
  {
    co_await f3();
    co_return OK;
  };

  auto f1 = [&f2]() -> Either<Void, LevelAError>
  {
    co_await f2();
    co_return OK;
  };

  auto result = f1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 500);
  EXPECT_EQ(result.error()->message, "Internal");
}

TEST(M1S13_EitherCrossTypeErrorPropagation, DISABLED_U11_DerivedThenConv_Nested)
{
  RecordProperty("id", "M1-S13-U12");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "ConvertibleToNetworkError propagates through f1->f2->f3 nested chain "
      "(E2 derived from E1, E3 conv E2)");

  auto f3 = []() -> Either<Void, ConvertibleToNetworkError>
  {
    co_return ConvertibleToNetworkError{
        504, "Gateway Timeout", "/upstream", "conn-refused"};
  };

  auto f2 = [&f3]() -> Either<Void, NetworkError>
  {
    co_await f3();
    co_return OK;
  };

  auto f1 = [&f2]() -> Either<Void, BaseError>
  {
    co_await f2();
    co_return OK;
  };

  auto result = f1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 504);
  EXPECT_EQ(result.error()->message, "Gateway Timeout");
}

TEST(
    M1S13_EitherCrossTypeErrorPropagation,
    DISABLED_U12_ConvertibleError_Destructor)
{
  RecordProperty("id", "M1-S13-U13");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "Destructor called correctly for convertible (non-derived) error "
      "after cross-type propagation");

  using BEDT = ErrorDestructorTracker<"M1-S13-U13">;
  using CDT = ConvertibleDestructorTracker<"M1-S13-U13">;

  auto returnConvertibleError = [](int code,
                                   std::string msg) -> Either<int, CDT>
  { co_return CDT{code, std::move(msg)}; };

  CDT::reset();
  BEDT::reset();

  {
    auto outer = [&returnConvertibleError]() -> Either<int, BEDT>
    {
      int val = co_await returnConvertibleError(404, "Not found");
      co_return val;
    };

    auto result = outer();
    ASSERT_TRUE(result.done());
    ASSERT_TRUE(result.error());
    ASSERT_FALSE(result.value());
    EXPECT_EQ(result.error()->code, 404);
  }

  EXPECT_GE(CDT::s_destructorCount, 1);
  EXPECT_GE(BEDT::s_destructorCount, 1);
}

TEST(
    M1S13_EitherCrossTypeErrorPropagation, U13_MultiInheritance_FirstParentPath)
{
  RecordProperty("id", "M1-S13-U05");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "AuditableApiError propagates to Either<string, BaseError> preserving "
      "BaseError polymorphism");

  auto level2 = []() -> Either<int, AuditableApiError>
  {
    co_return AuditableApiError{
        503, "Service Down", 1, "health-check", "payment-svc"};
  };

  auto level1 = [&level2]() -> Either<std::string, BaseError>
  {
    int val = co_await level2();
    co_return std::to_string(val);
  };

  auto result = level1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->code, 503);
  EXPECT_EQ(result.error()->describe(), "Service Down [payment-svc]");
}

TEST(
    M1S13_EitherCrossTypeErrorPropagation,
    DISABLED_U14_MultiInheritance_SecondParentPath)
{
  RecordProperty("id", "M1-S13-U14");
  RecordProperty("ver", "0.05");
  RecordProperty(
      "desc",
      "AuditableApiError propagates to Either<string, AuditableError> "
      "preserving AuditableError polymorphism");

  auto level2 = []() -> Either<int, AuditableApiError>
  {
    co_return AuditableApiError{
        503, "Service Down", 1, "health-check", "payment-svc"};
  };

  auto level1 = [&level2]() -> Either<std::string, AuditableError>
  {
    int val = co_await level2();
    co_return std::to_string(val);
  };

  auto result = level1();
  ASSERT_TRUE(result.done());
  ASSERT_TRUE(result.error());
  ASSERT_FALSE(result.value());
  EXPECT_EQ(result.error()->severity, 1);
  EXPECT_EQ(result.error()->audit(), "[severity=1] health-check [payment-svc]");
}

// NOLINTEND(readability-magic-numbers)
