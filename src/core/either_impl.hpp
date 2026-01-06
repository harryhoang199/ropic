// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <coroutine>
#include <variant>

#include "attributes.hpp"
#include "borrower.hpp"
#include "either_concept.hpp"

namespace ropic::detail
{
/**
 * @class EitherImpl
 * @brief Coroutine-based Railway Oriented Programming type: holds either data,
 * error, or empty state.
 *
 * @tparam DATA The success value type (must satisfy detail::plain_value_type)
 * @tparam ERROR The error type (must satisfy detail::plain_value_type). Must
 * differ from DATA.
 *
 * Operates in two modes:
 * - **Value Mode**: Direct container for error/data.
 * - **Coroutine Mode**: Return type for coroutines; values stored in promise.
 *
 * @warning The `data()` and `error()` methods return Borrower pointers that
 * become dangling after the EitherImpl object is destroyed or moved.
 *
 * @code
 * // Coroutine with automatic error propagation
 * EitherImpl<int, Error> compute(int x) {
 *     if (x < 0) co_return Error{ErrorTag::VALIDATION, "Negative"};
 *     co_return x * 2;
 * }
 *
 * // Composing coroutines - errors propagate automatically via co_await
 * EitherImpl<int, Error> composed(int x) {
 *     auto value = co_await compute(x);
 *     co_return value + 10;
 * }
 *
 * // Checking results
 * auto result = compute(5);
 * if (auto err = result.error()) {
 *     std::cout << err->message();
 * } else {
 *     std::cout << *result.data();
 * }
 * @endcode
 */
template <typename DATA, typename ERROR>
class ROPIC_CORO_AWAIT_ELIDABLE EitherImpl
{
  static_assert(
      either_concept<DATA, ERROR>,
      "`DATA` and `ERROR` must not be identical and not be reference, const, "
      "void or monostate types");
  // ==========================================
  // PRIVATE NESTED TYPES
  // ==========================================
  class Promise;

  /// Awaiter for non-EitherImpl coroutines (Task, Generator). Returns
  /// EitherImpl as-is.
  template <bool IS_LVALUE>
  class InteropAwaiter;

  /// Awaiter for EitherImpl-to-EitherImpl composition. Propagates errors,
  /// extracts values.
  template <typename OTHER, bool IS_LVALUE>
  class PropagatingAwaiter;

  using Handle = std::coroutine_handle<Promise>;

  template <typename OTHER, bool IS_LVALUE>
  using AwaitableEither = std::conditional_t<
      IS_LVALUE,
      EitherImpl<OTHER, ERROR>&,   // lvalue ref when true
      EitherImpl<OTHER, ERROR>&&>; // rvalue ref when false

  // ==========================================
  // PRIVATE VARIABLES & FUNCTIONS
  // ==========================================
  /// Coroutine handle (null in value mode)
  Handle _handle;

  /// Holds empty, data, or error
  std::variant<std::monostate, DATA, ERROR> _result;

  /// @brief Constructs an EitherImpl from a coroutine handle (coroutine mode).
  explicit EitherImpl(Handle h) noexcept : _handle(h), _result(std::monostate{})
  {
    _handle.promise().setEither(this);
  }

  void _setErrorAndNullifyHandle(ERROR&& value)
      noexcept(std::is_nothrow_move_assignable_v<ERROR>)
  {
    _result = std::move(value);
    _handle = nullptr;
  }

  void _setDataAndNullifyHandle(DATA&& value)
      noexcept(std::is_nothrow_move_assignable_v<DATA>)
  {
    _result = std::move(value);
    _handle = nullptr;
  }

public:
  using promise_type = Promise;

  // ==========================================
  // CONSTRUCTORS, DESTRUCTOR, OPERATORS
  // ==========================================

  /// @brief Constructs an EitherImpl containing an error.
  EitherImpl(ERROR e) noexcept(std::is_nothrow_move_constructible_v<ERROR>)
      : _handle(nullptr), _result(std::move(e))
  {
  }

  /// @brief Constructs an EitherImpl containing data.
  EitherImpl(DATA d) noexcept(std::is_nothrow_move_constructible_v<DATA>)
      : _handle(nullptr), _result(std::move(d))
  {
  }

  /// @brief Copy disabled; use move semantics.
  EitherImpl(const EitherImpl&) = delete;

  /// @brief Copy disabled; use move semantics.
  auto operator=(const EitherImpl&) -> EitherImpl& = delete;

  /// @brief Destroy handle if not null
  ~EitherImpl() noexcept
  {
    if (_handle)
      _handle.destroy();
  }

  /// @brief Move constructor; transfers ownership of handle and result.
  /// Updates the promise's Either pointer if coroutine is still active.
  EitherImpl(EitherImpl&& other) noexcept(
      std::is_nothrow_move_constructible_v<DATA>
      && std::is_nothrow_move_constructible_v<ERROR>)
      : _handle(other._handle), _result(std::move(other._result))
  {
    // Update promise to point to new location (critical for async coroutines)
    if (_handle)
      _handle.promise().setEither(this);

    other._handle = nullptr;
    other._result = std::monostate{};
  }

  /// @brief Move assignment operator.
  /// Updates the promise's Either pointer if coroutine is still active.
  auto operator=(EitherImpl&& other) noexcept(
      std::is_nothrow_move_assignable_v<DATA>
      && std::is_nothrow_move_assignable_v<ERROR>) -> EitherImpl&
  {
    if (this != &other)
    {
      if (_handle)
        _handle.destroy();
      _handle = other._handle;

      // Update promise to point to new location (critical for async coroutines)
      if (_handle)
        _handle.promise().setEither(this);

      other._handle = nullptr;

      _result = std::move(other._result);
      other._result = std::monostate{};
    }
    return *this;
  }

  /// @brief Awaitable for lvalue; returns EitherImpl reference via
  /// InteropAwaiter.
  [[nodiscard]]
  auto operator co_await() & noexcept -> InteropAwaiter<true>
  {
    return InteropAwaiter<true>{*this};
  }

  /// @brief Awaitable for rvalue; returns EitherImpl by move via
  /// InteropAwaiter.
  [[nodiscard]]
  auto operator co_await() && noexcept -> InteropAwaiter<false>
  {
    return InteropAwaiter<false>{std::move(*this)};
  }

  // ==========================================
  // ACCESSORS
  // ==========================================

  /**
   * @brief Returns optional reference to error if present, empty Borrower
   * otherwise.
   * @return Borrower<ERROR> containing error reference, or empty Borrower.
   * @warning Returned Borrower becomes dangling after EitherImpl is destroyed
   * or moved.
   *
   * For coroutine mode, extracts result from promise on first access.
   */
  [[nodiscard]]
  auto error() noexcept -> Borrower<ERROR>
  {
    return Borrower<ERROR>{std::get_if<ERROR>(&_result)};
  }

  /**
   * @brief Returns optional reference to data if present, empty Borrower
   * otherwise.
   * @return Borrower<DATA> containing data reference, or empty Borrower.
   * @warning Returned Borrower becomes dangling after EitherImpl is destroyed
   * or moved.
   *
   * For coroutine mode, extracts result from promise on first access.
   */
  [[nodiscard]]
  auto data() noexcept -> Borrower<DATA>
  {
    return Borrower<DATA>{std::get_if<DATA>(&_result)};
  }

  [[nodiscard]]
  auto done() const noexcept -> bool
  {
    return !std::holds_alternative<std::monostate>(_result);
  }
};
} // namespace ropic::detail

#include "either_awaiters.inl"
#include "either_promise.inl"
