// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include "function_traits.hpp"

namespace ropic::detail
{
/// @brief Type that can signal resumption of a suspended coroutine.
///
/// Requires a `requestResume()` method, matching the interface of
/// `ropic::ResumeSource`.
template <typename S>
concept resume_source = requires(S rs) {
  { rs.requestResume() };
};

/// @brief Awaiter whose `await_suspend` parameter is
/// `resume_source`.
///
/// Checks the standard awaiter requirements (`await_ready`,
/// `await_suspend`, `await_resume`) and additionally verifies that
/// the parameter type of `await_suspend` satisfies
/// `resume_source` (deduced via `extractArg` helpers).
template <typename A>
concept safe_awaiter = requires(std::decay_t<A> t) {
  // Standard Awaiter requirements
  { t.await_ready() } -> std::convertible_to<bool>;
  { t.await_resume() };

  // Check for the existence of await_suspend first (by taking its address).
  &std::decay_t<A>::await_suspend;

  // --- ARGUMENT EXTRACTION LOGIC ---
  // "Call" the dummy extractArg function inside decltype.
  // This forces the compiler to deduce type 'A' (the parameter of
  // await_suspend).
  requires(
      FunctionTraits<decltype(&std::decay_t<A>::await_suspend)>::arity >= 1);
  requires resume_source<typename FunctionTraits<
      decltype(&std::decay_t<A>::await_suspend)>::template ArgType<0>>;
};

/// @brief Type with member operator co_await() returning a ropic awaiter.
template <typename A>
concept member_co_awaitable = requires(std::decay_t<A> t) {
  { t.operator co_await() };
  requires safe_awaiter<decltype(t.operator co_await())>;
};

/// @brief Type with non-member operator co_await() returning a ropic awaiter.
template <typename A>
concept non_member_co_awaitable = requires(A t) {
  { operator co_await(t) };
  requires safe_awaiter<decltype(operator co_await(t))>;
};

/// @brief Type with any operator co_await() returning a ropic awaiter.
template <typename A>
concept co_awaitable = member_co_awaitable<A> || non_member_co_awaitable<A>;

/// @brief Invokes `operator co_await` on an awaitable, preferring
/// member over non-member.
///
/// Exists as a free function to ensure proper ADL lookup without
/// interference from enclosing class scopes.
///
/// @tparam A  A type satisfying `co_awaitable`.
/// @param awaitable  The awaitable to invoke `operator co_await` on.
/// @return The awaiter produced by the co_await operator.
template <typename A>
  requires co_awaitable<A>
inline auto invokeCoAwait(A&& awaitable) -> decltype(auto)
{
  if constexpr (member_co_awaitable<A>)
    return static_cast<A&&>(awaitable).operator co_await();

  if constexpr (non_member_co_awaitable<A>)
    return operator co_await(static_cast<A&&>(awaitable));
}
} // namespace ropic::detail

namespace ropic
{
/// @brief Non-Either type that is a ropic awaiter or has ropic co_await
/// operator.
template <typename A>
concept safe_awaitable = detail::safe_awaiter<A> || detail::co_awaitable<A>;
} // namespace ropic