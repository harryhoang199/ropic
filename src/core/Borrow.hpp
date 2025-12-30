// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <type_traits>
#include <cassert>

#ifdef NDEBUG
#define BORROWED_PTR_ASSERT(ptr) ((void)0)
#else
#define BORROWED_PTR_ASSERT(ptr) \
  assert((ptr) != nullptr && "Dereferencing a null borrowed pointer")
#endif

namespace ropic
{
  /**
   * @brief A non-owning, non-copyable pointer wrapper for scope-bound access.
   *
   * Prevents ownership misconceptions and discourages dangling pointer access by
   * being immovable. Debug builds assert on null dereference.
   *
   * @tparam T The pointed-to type. Must not be a reference type.
   * @warning The pointed-to object must outlive the Borrow instance.
   *
   * @code
   * if (Borrow<Error> err = result.error()) {
   *     std::cout << err->message() << "\n";
   * }
   * @endcode
   */
  template <typename T>
    requires(!std::is_reference_v<T>)
  class Borrow
  {
    T *_pointer;

  public:
    /// @brief Constructs a Borrow from a raw pointer (may be nullptr).
    explicit Borrow(T *pointer) noexcept : _pointer(pointer) {}

    Borrow(Borrow const &) = delete;
    Borrow(Borrow &&) = delete;
    auto operator=(Borrow const &) -> Borrow & = delete;
    auto operator=(Borrow &&) -> Borrow & = delete;

    /// @brief Returns true if the pointer is non-null.
    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
      return _pointer != nullptr;
    }

    /// @brief Member access. Asserts non-null in debug mode.
    [[nodiscard]] constexpr auto operator->() const noexcept -> T const *
    {
      BORROWED_PTR_ASSERT(_pointer);
      return _pointer;
    }

    /// @copydoc operator->() const
    [[nodiscard]] constexpr auto operator->() noexcept -> T *
    {
      BORROWED_PTR_ASSERT(_pointer);
      return _pointer;
    }

    /// @brief Dereference. Asserts non-null in debug mode.
    [[nodiscard]] constexpr auto operator*() noexcept -> T &
    {
      BORROWED_PTR_ASSERT(_pointer);
      return *_pointer;
    }

    /// @copydoc operator*()
    [[nodiscard]] constexpr auto operator*() const noexcept -> T const &
    {
      BORROWED_PTR_ASSERT(_pointer);
      return *_pointer;
    }

    /// @brief Returns true if the pointer is null.
    [[nodiscard]] constexpr auto operator==(std::nullptr_t) const noexcept -> bool
    {
      return _pointer == nullptr;
    }
  };
} // namespace ropic
