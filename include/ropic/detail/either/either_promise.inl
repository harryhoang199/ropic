// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <concepts>
#include <coroutine>
#include <exception>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "ropic/safe_awaitable.hpp"

#include "ropic/detail/either/either_impl.hpp"
#include "ropic/detail/either/final_awaiter.hpp"
#include "ropic/detail/either/promise_chain_node.hpp"
#include "ropic/detail/either/safe_awaitable_adapter.hpp"

namespace ropic::detail
{
/// Awaiter for EitherImpl-to-EitherImpl composition. Propagates errors,
/// extracts values.
template <
    typename VALUE,
    typename ERROR,
    typename OTHER_VAL,
    typename DERIVED_ERR,
    bool IS_EITHER_LREF>
class PropagatingAwaiter;

/**
 * @brief Promise type for Either coroutines.
 *
 * Controls the coroutine lifecycle: immediate start (no initial suspend),
 * symmetric-transfer final suspend, and stores co_return values directly
 * into the associated EitherImpl.
 *
 * Inherits PromiseChainNode to participate in type-erased continuation
 * chains for error propagation and suspension tracking.
 *
 * @see PromiseChainNode — base providing continuation, error, resumeTarget.
 * @see FinalAwaiter — returned by final_suspend() for symmetric transfer.
 */
template <typename VALUE, typename ERROR>
class EitherImpl<VALUE, ERROR>::Promise : public PromiseChainNode
{
public:
  using PromiseChainNode::continuation; ///< Inherited continuation handle.
  using PromiseChainNode::error;        ///< Inherited error pointer (void*).
  using PromiseChainNode::resumeTarget; ///< Inherited ResumeTarget.

  /// @brief Storage for successful result. Empty until co_return VALUE.
  std::optional<VALUE> result;

  /// @brief Destructor. Deletes the heap-allocated error if present.
  /// @note Uses static_cast<ERROR*> to recover the concrete type from
  /// the type-erased void* stored in PromiseChainNode::error.
  ~Promise() { delete static_cast<ERROR*>(error); }

  // NOLINTBEGIN(readability-identifier-naming)
  /// @brief Creates the EitherImpl bound to this promise's coroutine handle.
  /// @return An EitherImpl owning this coroutine frame.
  [[nodiscard]]
  auto get_return_object() noexcept -> EitherImpl
  {
    return EitherImpl{std::coroutine_handle<Promise>::from_promise(*this)};
  }

  /// @brief Starts execution immediately (no initial suspend).
  /// @return `std::suspend_never` — the coroutine runs eagerly.
  [[nodiscard]]
  auto initial_suspend() const noexcept -> std::suspend_never
  {
    return {};
  }

  /// @brief Handles `co_return std::tuple{...}` by unpacking the tuple
  /// into a VALUE or ERROR constructor.
  ///
  /// @param args  Tuple whose elements are forwarded to VALUE or ERROR.
  ///
  /// @note Fails at compile time if the tuple arguments can construct
  /// both VALUE and ERROR (ambiguous) or neither.
  template <typename... ARGS>
  void return_value(std::tuple<ARGS...> args) noexcept(
      std::is_constructible_v<VALUE, ARGS...>
          ? std::is_nothrow_constructible_v<VALUE, ARGS...>
          : std::is_nothrow_constructible_v<ERROR, ARGS...>)
  {
    if constexpr (
        std::is_constructible_v<VALUE, ARGS...>
        && std::is_constructible_v<ERROR, ARGS...>)
    {
      static_assert(
          false,
          "Promise::return_value: tuple arguments must not construct both "
          "VALUE and ERROR (ambiguous return type)");
    }
    else if constexpr (std::is_constructible_v<VALUE, ARGS...>)
    {
      std::apply(
          [this](auto&&... unpackedArgs)
          {
            result.emplace(
                std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
          },
          args);
    }
    else if constexpr (std::is_constructible_v<ERROR, ARGS...>)
    {
      error = std::apply(
          [](auto&&... unpackedArgs) -> ERROR*
          {
            return new ERROR(
                std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
          },
          args);
    }
    else
    {
      static_assert(
          false,
          "Promise::return_value: tuple arguments must construct VALUE or "
          "ERROR (verify constructors exist)");
    }
  }

  /// @brief Handles `co_return` with a const lvalue VALUE.
  /// @param v  The value to store.
  void return_value(VALUE const& v)
      noexcept(std::is_nothrow_constructible_v<VALUE, VALUE const&>)
    requires std::is_constructible_v<VALUE, VALUE const&>
  {
    result.emplace(v);
  }

  /// @brief Handles `co_return` with an rvalue VALUE.
  /// @param v  The value to move-store.
  void return_value(VALUE&& v)
      noexcept(std::is_nothrow_constructible_v<VALUE, VALUE&&>)
    requires std::is_constructible_v<VALUE, VALUE&&>
  {
    result.emplace(std::move(v));
  }

  /// @brief Handles `co_return` with a const lvalue ERROR.
  /// @param e  The error to heap-allocate and store.
  void return_value(ERROR const& e)
      noexcept(std::is_nothrow_constructible_v<ERROR, ERROR const&>)
    requires std::is_constructible_v<ERROR, ERROR&>
  {
    error = new ERROR(e);
  }

  /// @brief Handles `co_return` with an rvalue ERROR.
  /// @param e  The error to heap-allocate and move-store.
  void return_value(ERROR&& e)
      noexcept(std::is_nothrow_constructible_v<ERROR, ERROR&&>)
    requires std::is_constructible_v<ERROR, ERROR&&>
  {
    error = new ERROR(std::move(e));
  }

  /// @brief Handles `co_return` with a type derived from ERROR.
  ///
  /// @tparam DERIVED_ERR  A type derived from ERROR.
  /// @param e  The derived error to heap-allocate.
  ///
  /// @note Allocates the derived type on the heap to preserve
  /// polymorphic behavior through the type-erased void* chain.
  template <typename DERIVED_ERR>
    requires std::derived_from<std::decay_t<DERIVED_ERR>, ERROR>
          && std::is_constructible_v<std::decay_t<DERIVED_ERR>, DERIVED_ERR>
  void return_value(DERIVED_ERR&& e) noexcept(
      std::is_nothrow_constructible_v<std::decay_t<DERIVED_ERR>, DERIVED_ERR>)
  {
    error = new std::decay_t<DERIVED_ERR>(std::forward<DERIVED_ERR>(e));
  }

  /// @brief Final suspension point. Handles error propagation and symmetric
  /// transfer.
  ///
  /// - No error + has continuation: symmetric transfer to caller.
  /// - No error + no continuation: noop (top-level coroutine completed).
  /// - Error + has continuation: walks chain to tail, transfers error
  ///   ownership, then noop.
  /// - Error + no continuation: noop (error stays on this promise).
  ///
  /// @return FinalAwaiter configured for the appropriate transfer.
  [[nodiscard]]
  auto final_suspend() noexcept -> FinalAwaiter
  {
    if (error == nullptr)
    {
      if (continuation)
      {
        return FinalAwaiter{continuation};
      }
    }
    else if (auto currentCont = continuation)
    {
      // Walk to the tail of the continuation chain
      while (auto nextCont = currentCont.promise().continuation)
      {
        currentCont = nextCont;
      }
      // Transfer error ownership to the tail promise
      currentCont.promise().error = error;
      error = nullptr;
    }

    return FinalAwaiter{std::noop_coroutine()};
  }

  /// @brief Terminates on unhandled exceptions.
  void unhandled_exception() const noexcept { std::terminate(); }

  /// @brief Transforms a non-Either awaitable for use inside an Either
  /// coroutine.
  ///
  /// If the awaitable satisfies `safe_awaitable`, wraps it in a
  /// SafeAwaitableAdapter to integrate with the resume mechanism.
  /// Otherwise, passes it through unchanged.
  ///
  /// @tparam SAFE_AWT  The awaitable type (must not be an EitherImpl).
  /// @param awaitable  The awaitable to transform.
  /// @return SafeAwaitableAdapter or the original awaitable.
  template <typename SAFE_AWT>
    requires(!IsEitherImpl<std::decay_t<SAFE_AWT>>::value)
  auto await_transform(SAFE_AWT&& awaitable) const noexcept -> decltype(auto)
  {
    if constexpr (safe_awaitable<SAFE_AWT>)
    {
      return SafeAwaitableAdapter<VALUE, ERROR, SAFE_AWT>{
          std::forward<SAFE_AWT>(awaitable)};
    }
    else
    {
      return static_cast<SAFE_AWT&&>(awaitable);
    }
  }

  /// @brief Transforms an lvalue EitherImpl into a PropagatingAwaiter
  /// for automatic error propagation.
  ///
  /// @tparam OTHER_VAL    Value type of the awaited Either.
  /// @tparam DERIVED_ERR  Error type of the awaited Either (must be
  ///                      convertible to ERROR).
  /// @param awaitable  The lvalue EitherImpl to co_await.
  /// @return PropagatingAwaiter that yields `OTHER_VAL&` on success.
  template <typename OTHER_VAL, typename DERIVED_ERR>
    requires(std::convertible_to<DERIVED_ERR, ERROR>)
  auto await_transform(
      EitherImpl<OTHER_VAL, DERIVED_ERR>& awaitable) const noexcept
      -> PropagatingAwaiter<VALUE, ERROR, OTHER_VAL, DERIVED_ERR, true>
  {
    return PropagatingAwaiter<VALUE, ERROR, OTHER_VAL, DERIVED_ERR, true>{
        awaitable._handle};
  }

  /// @brief Transforms an rvalue EitherImpl into a PropagatingAwaiter
  /// for automatic error propagation.
  ///
  /// @tparam OTHER_VAL    Value type of the awaited Either.
  /// @tparam DERIVED_ERR  Error type of the awaited Either (must be
  ///                      convertible to ERROR).
  /// @param awaitable  The rvalue EitherImpl to co_await.
  /// @return PropagatingAwaiter that yields `OTHER_VAL&&` on success.
  template <typename OTHER_VAL, typename DERIVED_ERR>
    requires(std::convertible_to<DERIVED_ERR, ERROR>)
  auto await_transform(
      EitherImpl<OTHER_VAL, DERIVED_ERR>&& awaitable) const noexcept
      -> PropagatingAwaiter<VALUE, ERROR, OTHER_VAL, DERIVED_ERR, false>
  {
    return PropagatingAwaiter<VALUE, ERROR, OTHER_VAL, DERIVED_ERR, false>{
        awaitable._handle};
  }
  // NOLINTEND(readability-identifier-naming)
};
} // namespace ropic::detail

#include "ropic/detail/either/propagating_awaiter.inl"
