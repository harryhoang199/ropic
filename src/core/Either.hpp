// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "EitherPromise.inl"
#include "Borrower.hpp"
#include <cassert>

namespace ropic
{
  namespace detail
  {
    /// @brief Ensures a type is a plain value: not a reference, not const-qualified, not void,
    /// and not std::monostate.
    template <typename T>
    concept plain_value_type = std::same_as<T, std::remove_cvref_t<T>> &&
                               !std::same_as<T, std::monostate> &&
                               !std::is_void_v<T>;
  }

  /**
   * @class Either
   * @brief Coroutine-based Railway Oriented Programming type: holds either data, error, or empty state.
   *
   * @tparam DATA The success value type (must satisfy detail::plain_value_type)
   * @tparam ERROR The error type (must satisfy detail::plain_value_type). Must differ from DATA.
   *
   * Operates in two modes:
   * - **Value Mode**: Direct container for error/data.
   * - **Coroutine Mode**: Return type for coroutines; values stored in promise.
   *
   * @warning The `data()` and `error()` methods return Borrower pointers that become
   *          dangling after the Either object is destroyed or moved.
   *
   * @code
   * // Coroutine with automatic error propagation
   * Either<int, Error> compute(int x) {
   *     if (x < 0) co_return Error{ErrorTag::VALIDATION, "Negative"};
   *     co_return x * 2;
   * }
   *
   * // Composing coroutines - errors propagate automatically via co_await
   * Either<int, Error> composed(int x) {
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
  template <detail::plain_value_type DATA, detail::plain_value_type ERROR>
    requires(!std::is_same_v<DATA, ERROR>)
  class Either
  {
    using Handle = std::coroutine_handle<detail::EitherPromise<DATA, ERROR, Either>>;
    Handle _handle;                                    ///< Coroutine handle (null in value mode)
    std::variant<std::monostate, DATA, ERROR> _result; ///< Holds empty, data, or error

    void updateResult() noexcept
    {
      if (_handle)
      {
        _result = std::move(_handle.promise().result);
        _handle.destroy();
        _handle = nullptr;
      }
    }

  public:
    using promise_type = detail::EitherPromise<DATA, ERROR, Either>;
    // ==========================================
    // CONSTRUCTORS AND DESTRUCTOR
    // ==========================================

    /// @brief Constructs an Either containing an error.
    Either(ERROR e) noexcept(std::is_nothrow_move_constructible_v<ERROR>)
        : _handle(nullptr), _result(std::move(e)) {}

    /// @brief Constructs an Either containing data.
    Either(DATA d) noexcept(std::is_nothrow_move_constructible_v<DATA>)
        : _handle(nullptr), _result(std::move(d)) {}

    /// @brief Constructs an Either from a coroutine handle (coroutine mode).
    explicit Either(Handle h) noexcept
        : _handle(h), _result(std::monostate{}) {}

    /// @brief Destructor; destroys the coroutine handle if present.
    ~Either() noexcept
    {
      if (_handle)
        _handle.destroy();
    }

    Either(const Either &) = delete;
    auto operator=(const Either &) -> Either & = delete;

    /// @brief Move constructor; transfers ownership of handle and result.
    Either(Either &&other) noexcept(
        std::is_nothrow_move_constructible_v<DATA> &&
        std::is_nothrow_move_constructible_v<ERROR>)
        : _handle(other._handle), _result(std::move(other._result))
    {
      other._handle = nullptr;
      other._result = std::monostate{};
    }

    /// @brief Move assignment operator.
    auto operator=(Either &&other) noexcept(
        std::is_nothrow_move_assignable_v<DATA> &&
        std::is_nothrow_move_assignable_v<ERROR>) -> Either &
    {
      if (this != &other)
      {
        if (_handle)
          _handle.destroy();
        _handle = other._handle;
        other._handle = nullptr;
        _result = std::move(other._result);
        other._result = std::monostate{};
      }
      return *this;
    }

    // ==========================================
    // ACCESSORS
    // ==========================================

    /**
     * @brief Returns optional reference to error if present, empty Borrower otherwise.
     * @return Borrower<ERROR> containing error reference, or empty Borrower.
     * @warning Returned Borrower becomes dangling after Either is destroyed or moved.
     *
     * For coroutine mode, extracts result from promise on first access.
     */
    [[nodiscard]] auto error() noexcept -> Borrower<ERROR>
    {
      updateResult();
      return Borrower<ERROR>{std::get_if<ERROR>(&_result)};
    }

    /**
     * @brief Returns optional reference to data if present, empty Borrower otherwise.
     * @return Borrower<DATA> containing data reference, or empty Borrower.
     * @warning Returned Borrower becomes dangling after Either is destroyed or moved.
     *
     * For coroutine mode, extracts result from promise on first access.
     */
    [[nodiscard]] auto data() noexcept -> Borrower<DATA>
      requires(!std::is_same_v<DATA, Void>)
    {
      updateResult();
      return Borrower<DATA>{std::get_if<DATA>(&_result)};
    }
  };
}