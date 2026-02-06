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

#include "either_impl.hpp"
#include "final_awaiter.hpp"

namespace ropic::detail
{
/**
 * @brief Non-template base for coroutine continuation chains and type-erased
 * error storage.
 *
 * Enables heterogeneous error-type chains (e.g., PromiseBase<BaseError> →
 * PromiseBase<NetworkError>) by storing both continuation and error as
 * type-erased members. All chain-walking and error-transfer code operates
 * through this single type, avoiding coroutine_handle type mismatches.
 */
struct PromiseChainNode
{
  /// @brief Handle to the next coroutine in the chain (for error propagation).
  std::coroutine_handle<PromiseChainNode> continuation = nullptr;

  /// @brief Heap-allocated error (type-erased), or nullptr if no error.
  /// Owned by the concrete Promise that inherits from this node.
  void* error = nullptr;
};

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
 * Controls coroutine lifecycle: immediate start, no final suspend,
 * and stores co_return values directly into the associated EitherImpl.
 */
template <typename VALUE, typename ERROR>
class EitherImpl<VALUE, ERROR>::Promise : public PromiseChainNode
{
public:
  using PromiseChainNode::continuation; ///< Inherited continuation handle.
  using PromiseChainNode::error;        ///< Inherited error pointer (void*).

  /// @brief Storage for successful result. Empty until co_return VALUE.
  std::optional<VALUE> result;

  /// @brief Destructor. Deletes heap-allocated error if present.
  ~Promise() { delete static_cast<ERROR*>(error); }

  // NOLINTBEGIN(readability-identifier-naming)
  /// @brief Creates EitherImpl bound to this promise's coroutine handle.
  [[nodiscard]]
  auto get_return_object() noexcept -> EitherImpl
  {
    return EitherImpl{std::coroutine_handle<Promise>::from_promise(*this)};
  }

  /// @brief Starts execution immediately (no initial suspend).
  [[nodiscard]]
  auto initial_suspend() const noexcept -> std::suspend_never
  {
    return {};
  }

  /// @brief Handles co_return std::make_tuple(...) for in-place VALUE
  /// construction.
  /// @note Enables immovable types via: `co_return std::make_tuple(args...);`
  template <typename... ARGS>
    requires std::is_constructible_v<VALUE, ARGS...>
  void return_value(std::tuple<ARGS...> args)
      noexcept(std::is_nothrow_constructible_v<VALUE, ARGS...>)
  {
    std::apply(
        [this](auto&&... unpackedArgs)
        {
          result.emplace(std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
        },
        args);
  }

  /// @brief Handles co_return std::make_tuple(...) for in-place ERROR
  /// construction.
  /// @note Enables immovable error types via: `co_return std::make_tuple(...);`
  template <typename... ARGS>
    requires std::is_constructible_v<ERROR, ARGS...>
  void return_value(std::tuple<ARGS...> args)
      noexcept(std::is_nothrow_constructible_v<ERROR, ARGS...>)
  {
    error = std::apply(
        [](auto&&... unpackedArgs) -> ERROR*
        {
          return new ERROR(
              std::forward<decltype(unpackedArgs)>(unpackedArgs)...);
        },
        args);
  }

  /// @brief Handles co_return with a VALUE value.
  void return_value(VALUE const& v)
      noexcept(std::is_nothrow_constructible_v<VALUE, VALUE const&>)
    requires std::is_constructible_v<VALUE, VALUE&>
  {
    result.emplace(v);
  }

  /// @brief Handles co_return with a VALUE value.
  void return_value(VALUE&& v)
      noexcept(std::is_nothrow_constructible_v<VALUE, VALUE&&>)
    requires std::is_constructible_v<VALUE, VALUE&&>
  {
    result.emplace(std::move(v));
  }

  /// @brief Handles co_return with an ERROR value.
  void return_value(ERROR const& e)
      noexcept(std::is_nothrow_constructible_v<ERROR, ERROR const&>)
    requires std::is_constructible_v<ERROR, ERROR&>
  {
    error = new ERROR(e);
  }

  /// @brief Handles co_return with an ERROR value.
  void return_value(ERROR&& e)
      noexcept(std::is_nothrow_constructible_v<ERROR, ERROR&&>)
    requires std::is_constructible_v<ERROR, ERROR&&>
  {
    error = new ERROR(std::move(e));
  }

  /// @brief Handles co_return with a type derived from ERROR.
  /// @note Preserves polymorphic behavior by allocating the derived type.
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
  /// If no error: resumes continuation (symmetric transfer) or returns no-op.
  /// If error: walks continuation chain to find root, transfers error
  /// ownership, then returns no-op to destroy this frame.
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
      // Walk to the root of the continuation chain
      while (auto nextCont = currentCont.promise().continuation)
      {
        currentCont = nextCont;
      }
      // Transfer error ownership to root promise
      currentCont.promise().error = error;
      error = nullptr;
    }

    return FinalAwaiter{std::noop_coroutine()};
  }

  /// @brief Terminates on unhandled exceptions.
  void unhandled_exception() const noexcept { std::terminate(); }

  /// @brief Pass-through for types not matching specialized overloads.
  template <typename T>
    requires(!IsEitherImpl<std::decay_t<T>>::value)
  auto await_transform(T&& awaitable) const noexcept -> T&&
  {
    return static_cast<T&&>(awaitable);
  }

  /// @brief Transforms rvalue or lvalue EitherImpl to PropagatingAwaiter for
  /// error propagation.
  template <typename OTHER_VAL, typename DERIVED_ERR>
    requires(std::derived_from<DERIVED_ERR, ERROR>)
  auto await_transform(
      EitherImpl<OTHER_VAL, DERIVED_ERR>& awaitable) const noexcept
      -> PropagatingAwaiter<VALUE, ERROR, OTHER_VAL, DERIVED_ERR, true>
  {
    return PropagatingAwaiter<VALUE, ERROR, OTHER_VAL, DERIVED_ERR, true>{
        awaitable._handle};
  }

  /// @copydoc await_transform(EitherImpl<OTHER_VAL, ERROR>&&)
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

#include "propagating_awaiter.inl"
