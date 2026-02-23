// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "function_traits.hpp"
#include "safe_awaitable.hpp"

namespace ropic
{

/// @brief Primary template — undefined for non-awaitable types.
template <typename A>
struct SafeAwaiterTraits;

/// @brief Specialization for types satisfying `safe_awaiter`.
///
/// Extracts type information directly from the awaiter's
/// member functions.
template <typename A>
  requires(detail::safe_awaiter<A>)
struct SafeAwaiterTraits<A>
{
private:
  using SuspendTraits =
      FunctionTraits<decltype(&std::decay_t<A>::await_suspend)>;
  using ResumeTraits = FunctionTraits<decltype(&std::decay_t<A>::await_resume)>;

public:
  /// @brief The awaiter class type itself.
  using Awaiter = A;

  /// @brief Parameter type of `await_suspend`.
  using SuspendArgType = typename SuspendTraits::template ArgType<0>;

  /// @brief Return type of `await_suspend`.
  using SuspendReturnType = typename SuspendTraits::ReturnType;

  /// @brief Return type of `await_resume`.
  using ResumeReturnType = typename ResumeTraits::ReturnType;
};

/// @brief Specialization for types with member `operator co_await()`.
///
/// Inherits all aliases from the `safe_awaiter` specialization of the awaiter
/// returned by `operator co_await()`.
template <typename A>
  requires(!detail::safe_awaiter<A> && detail::member_co_awaitable<A>)
struct SafeAwaiterTraits<A>
    : SafeAwaiterTraits<typename FunctionTraits<
          decltype(&std::decay_t<A>::operator co_await)>::ReturnType>
{
};

/// @brief Specialization for types with non-member `operator co_await()`.
///
/// Inherits all aliases from the `safe_awaiter` specialization of the awaiter
/// returned by the non-member `operator co_await()`.
template <typename A>
  requires(
      !detail::safe_awaiter<A>
      && !detail::member_co_awaitable<A>
      && detail::non_member_co_awaitable<A>)
struct SafeAwaiterTraits<A>
    : SafeAwaiterTraits<decltype(operator co_await(std::declval<A>()))>
{
};

} // namespace ropic
