// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <coroutine>
#include <utility>

#include "../shared/attributes.hpp"
#include "../shared/distinct_unqualified_types.hpp"

#include "ropic/borrower.hpp"

namespace ropic::detail
{
/// Awaiter for non-EitherImpl coroutines (Task, Generator). Returns
/// EitherImpl as-is.
template <typename VALUE, typename ERROR, bool IS_EITHER_LREF>
class InteropAwaiter;

/**
 * @class EitherImpl
 * @brief Coroutine-based Railway Oriented Programming type: holds either
 * data, error, or empty state.
 *
 * @tparam VALUE The success value type (must satisfy
 * detail::unqualified_type)
 * @tparam ERROR The error type (must satisfy
 * detail::unqualified_type). Must differ from VALUE.
 *
 * Operates in two modes:
 * - **Value Mode**: Direct container for error/data.
 * - **Coroutine Mode**: Return type for coroutines; values stored in promise.
 *
 * @warning The `value()` and `error()` methods return Borrower pointers that
 * become dangling after the EitherImpl object is destroyed or moved.
 */
template <typename VALUE, typename ERROR>
class ROPIC_CORO_AWAIT_ELIDABLE EitherImpl
{
  static_assert(
      distinct_unqualified_types<VALUE, ERROR>,
      "`VALUE` and `ERROR` must not be identical and not be reference, const, "
      "void or monostate types");

  // ==========================================
  // FRIEND DECLARATIONS
  // ==========================================
  friend struct EitherHelper;
  /// Allow all EitherImpl specializations to access each other's private
  /// members (needed for PropagatingAwaiter to transfer suspension state across
  /// types)
  template <typename OTHER_DATA, typename OTHER_ERROR>
  friend class EitherImpl;

  // ==========================================
  // PRIVATE NESTED TYPES
  // ==========================================
  class Promise;

  // ==========================================
  // PRIVATE VARIABLES & FUNCTIONS
  // ==========================================
  std::coroutine_handle<Promise> _handle;

  /// @brief Constructs an EitherImpl from a coroutine handle (coroutine
  /// mode).
  explicit EitherImpl(std::coroutine_handle<Promise> h) noexcept
      : _handle(h)
  {
  }

public:
  /// @brief Promise type for coroutine machinery. Defined in
  /// either_promise.inl.
  using promise_type = Promise;

  // ==========================================
  // CONSTRUCTORS, DESTRUCTOR, OPERATORS
  // ==========================================

  /// @brief Copy disabled; use move semantics.
  EitherImpl(const EitherImpl&) = delete;

  /// @brief Copy disabled; use move semantics.
  auto operator=(const EitherImpl&) -> EitherImpl& = delete;

  /// @brief Destructor. Destroys the coroutine frame if handle is valid.
  /// @note After destruction, any Borrower obtained from value() or error()
  /// becomes dangling.
  ~EitherImpl() noexcept
  {
    if (_handle)
      _handle.destroy();
  }

  /// @brief Move constructor; transfers ownership of handle.
  /// @note Always noexcept: only transfers coroutine handle (raw pointer).
  EitherImpl(EitherImpl&& other) noexcept
      : _handle{other._handle}
  {
    other._handle = nullptr;
  }

  /// @brief Move assignment operator; transfers ownership of handle.
  /// @note Always noexcept: only transfers coroutine handle (raw pointer).
  auto operator=(EitherImpl&& other) noexcept -> EitherImpl&
  {
    if (this != &other)
    {
      if (_handle)
        _handle.destroy();
      _handle = other._handle;
      other._handle = nullptr;
    }
    return *this;
  }

  /// @brief Awaitable for lvalue; returns EitherImpl reference via
  /// InteropAwaiter.
  [[nodiscard]]
  auto operator co_await() & noexcept -> InteropAwaiter<VALUE, ERROR, true>
  {
    return InteropAwaiter<VALUE, ERROR, true>{*this};
  }

  /// @brief Awaitable for rvalue; returns EitherImpl by move via
  /// InteropAwaiter.
  [[nodiscard]]
  auto operator co_await() && noexcept -> InteropAwaiter<VALUE, ERROR, false>
  {
    return InteropAwaiter<VALUE, ERROR, false>{std::move(*this)};
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
   */
  [[nodiscard]]
  auto error() const noexcept -> Borrower<ERROR>
  {
    if (_handle != nullptr)
      return Borrower<ERROR>{static_cast<ERROR*>(_handle.promise().error)};

    return Borrower<ERROR>{nullptr};
  }

  /**
   * @brief Returns optional reference to data if present, empty Borrower
   * otherwise.
   * @return Borrower<VALUE> containing data reference, or empty Borrower.
   * @warning Returned Borrower becomes dangling after EitherImpl is destroyed
   * or moved.
   */
  [[nodiscard]]
  auto value() const noexcept -> Borrower<VALUE>
  {
    if (_handle != nullptr)
    {
      auto& result = _handle.promise().result;
      if (result.has_value())
        return Borrower<VALUE>{&(result.value())};
    }

    return Borrower<VALUE>{nullptr};
  }

  /**
   * @brief Returns true if the EitherImpl contains a result (data or error).
   * @return true if data or error is present, false if suspended or moved-from.
   * @note Returns false for moved-from objects (handle is null).
   * @note Returns false while coroutine is suspended (async operations).
   */
  [[nodiscard]]
  auto done() const noexcept -> bool
  {
    if (_handle == nullptr)
      return false;

    return (_handle.promise().result.has_value())
        || (_handle.promise().error != nullptr);
  }
};

/**
 * @brief Type trait to detect EitherImpl specializations.
 */
template <typename T>
struct IsEitherImpl : std::false_type
{
};

template <typename DATA, typename ERROR>
struct IsEitherImpl<EitherImpl<DATA, ERROR>> : std::true_type
{
};
} // namespace ropic::detail

#include "either_promise.inl"
#include "interop_awaiter.inl"
