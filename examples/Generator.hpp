// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <coroutine>
#include <optional>
#include <utility>

namespace examples
{
// ==========================================
// GENERATOR COROUTINE TYPE
// ==========================================
// A Generator that yields values one at a time, demonstrating streaming
// patterns. Can be used to yield Either values for streaming error handling.

// NOLINTBEGIN(readability-identifier-naming)
// C++ coroutines require specific names: promise_type, yield_value, etc.

template <typename T>
struct Generator
{
  struct promise_type
  {
    std::optional<T> current_value;

    auto get_return_object() { return Generator{Handle::from_promise(*this)}; }
    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }
    void unhandled_exception() { std::terminate(); }
    void return_void() {}
    auto yield_value(T value)
    {
      current_value = std::move(value);
      return std::suspend_always{};
    }
  };

  using Handle = std::coroutine_handle<promise_type>;
  Handle handle;

  explicit Generator(Handle h) : handle(h) {}
  ~Generator()
  {
    if (handle)
      handle.destroy();
  }

  Generator(const Generator&) = delete;
  auto operator=(const Generator&) -> Generator& = delete;
  Generator(Generator&& other) noexcept : handle(other.handle)
  {
    other.handle = nullptr;
  }
  auto operator=(Generator&& other) noexcept -> Generator&
  {
    if (this != &other)
    {
      if (handle)
        handle.destroy();
      handle = other.handle;
      other.handle = nullptr;
    }
    return *this;
  }

  auto next() -> bool
  {
    if (!handle || handle.done())
      return false;
    handle.resume();
    return !handle.done();
  }

  auto value() -> T& { return *handle.promise().current_value; }
};

// NOLINTEND(readability-identifier-naming)

} // namespace examples
