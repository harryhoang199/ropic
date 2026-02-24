// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <coroutine>
#include <utility>

#include "ropic/borrower.hpp"
#include "ropic/either_state.hpp"

#include "ropic/detail/shared/attributes.hpp"
#include "ropic/detail/shared/distinct_unqualified_types.hpp"
#include "ropic/detail/shared/resume_phase.hpp"

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
 * @tparam VALUE The success value type (must satisfy detail::unqualified_type)
 * @tparam ERROR The error type (must satisfy detail::unqualified_type). Must
 * differ from VALUE.
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
      "EitherImpl: VALUE and ERROR must be distinct, non-reference, non-const, "
      "non-void types (use different types for success and error)");

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
  /// @param h  The coroutine handle produced by Promise::get_return_object().
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

  /// @brief Awaitable for lvalue; allows co_await from non-Either coroutines.
  /// @return InteropAwaiter that yields `EitherImpl&`.
  [[nodiscard]]
  auto operator co_await() & noexcept -> InteropAwaiter<VALUE, ERROR, true>
  {
    return InteropAwaiter<VALUE, ERROR, true>{*this};
  }

  /// @brief Awaitable for rvalue; allows co_await from non-Either coroutines.
  /// @return InteropAwaiter that yields `EitherImpl&&` (moved).
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
    assert(
        _handle
        && "EitherImpl::error: must not be called on moved-from or null "
           "object (ensure the object is not moved-from)");
    return Borrower<ERROR>{static_cast<ERROR*>(_handle.promise().error)};
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

    assert(
        _handle
        && "EitherImpl::value: must not be called on moved-from or null "
           "object (ensure the object is not moved-from)");

    auto& result = _handle.promise().result;
    if (result.has_value())
      return Borrower<VALUE>{&(result.value())};

    return Borrower<VALUE>{nullptr};
  }

  /**
   * @brief Returns the overall state of this Either coroutine.
   * @return CoroState indicating what actions are available.
   * @see CoroState for the meaning of each value.
   */
  [[nodiscard]]
  auto state() const noexcept -> CoroState
  {
    if (_handle == nullptr)
      return CoroState::UNDEFINED;

    if (_handle.promise().result.has_value()
        || (_handle.promise().error != nullptr))
      return CoroState::DONE;

    auto& resumeTarget = _handle.promise().resumeTarget;
    if (!resumeTarget)
      return CoroState::UNDEFINED;

    switch (resumeTarget.state())
    {
    case ResumePhase::SUSPENDED:
      return CoroState::PENDING;

    case ResumePhase::READY:
      return CoroState::READY;

    default:
      assert(
          false
          && "EitherImpl::state: RESUMED phase must not be observable "
             "(because resumeTarget is consumed after resume())");

      return CoroState::UNDEFINED;
    }
  }

  /**
   * @brief Waits for the suspended async operation to become ready,
   * then resumes the coroutine.
   *
   * @pre `state()` must return PENDING or READY.
   * @post The ResumeTarget is consumed; `state()` will subsequently
   *       return DONE or UNDEFINED.
   */
  void resume() noexcept
  {
    assert(
        _handle
        && "EitherImpl::resume: must not be called on moved-from or null "
           "object (ensure the object is not moved-from)");

    auto& resumeTarget = _handle.promise().resumeTarget;
    assert(
        resumeTarget
        && "EitherImpl::resume: must not be called without active "
           "ResumeTarget (verify state() returns PENDING or READY)");

    resumeTarget.waitAndResume();
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

#include "ropic/detail/either/either_promise.inl"
#include "ropic/detail/either/interop_awaiter.inl"
