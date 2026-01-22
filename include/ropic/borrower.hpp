// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <type_traits>

#define BORROWED_PTR_ASSERT(ptr)                                               \
  assert((ptr) != nullptr && "Dereferencing a null borrowed pointer")

namespace ropic
{
/**
 * @brief A non-owning, non-copyable pointer wrapper for scope-bound access.
 *
 * Prevents ownership misconceptions and discourages dangling pointer access by
 * being immovable. Debug builds assert on null dereference.
 *
 * @tparam T The pointed-to type. Must not be a reference type.
 * @warning The pointed-to object must outlive the Borrower instance.
 *
 * @code
 * if (Borrower<Error> err = result.error()) {
 *     std::cout << err->message() << "\n";
 * }
 * @endcode
 */
template <typename T>
  requires(!std::is_reference_v<T>)
class Borrower
{
  T* _pointer;

public:
  /// @brief Constructs a Borrower from a raw pointer (may be nullptr).
  explicit Borrower(T* pointer) noexcept : _pointer(pointer) {}

  Borrower(Borrower const&) = delete;
  Borrower(Borrower&&) = delete;
  auto operator=(Borrower const&) -> Borrower& = delete;
  auto operator=(Borrower&&) -> Borrower& = delete;

  /// @brief Returns true if the pointer is non-null.
  [[nodiscard]]
  constexpr explicit operator bool() const noexcept
  {
    return _pointer != nullptr;
  }

  /// @brief Member access. Asserts non-null in debug mode.
  [[nodiscard]]
  constexpr auto operator->() const noexcept -> T const*
  {
    BORROWED_PTR_ASSERT(_pointer);
    return _pointer;
  }

  /// @copydoc operator->() const
  [[nodiscard]]
  constexpr auto operator->() noexcept -> T*
  {
    BORROWED_PTR_ASSERT(_pointer);
    return _pointer;
  }

  /// @brief Dereference. Asserts non-null in debug mode.
  [[nodiscard]]
  constexpr auto operator*() noexcept -> T&
  {
    BORROWED_PTR_ASSERT(_pointer);
    return *_pointer;
  }

  /// @copydoc operator*()
  [[nodiscard]]
  constexpr auto operator*() const noexcept -> T const&
  {
    BORROWED_PTR_ASSERT(_pointer);
    return *_pointer;
  }

  /// @brief Returns true if the pointer is null.
  [[nodiscard]]
  constexpr auto operator==(std::nullptr_t) const noexcept -> bool
  {
    return _pointer == nullptr;
  }

  /// @brief Returns the raw pointer.
  [[nodiscard]]
  constexpr auto get() const noexcept -> T const*
  {
    return _pointer;
  }

  /// @copydoc get()
  [[nodiscard]]
  constexpr auto get() noexcept -> T*
  {
    return _pointer;
  }

  /// @brief Returns a reference to the pointed-to value.
  [[nodiscard]]
  constexpr auto value() const noexcept -> T const&
  {
    BORROWED_PTR_ASSERT(_pointer);
    return *_pointer;
  }

  /// @copydoc value()
  [[nodiscard]]
  constexpr auto value() noexcept -> T&
  {
    BORROWED_PTR_ASSERT(_pointer);
    return *_pointer;
  }
};
} // namespace ropic
