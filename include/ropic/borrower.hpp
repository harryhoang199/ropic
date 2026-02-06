// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <cassert>
#include <type_traits>

/**
 * @def BORROWED_PTR_ASSERT
 * @brief Debug assertion for null pointer dereference in Borrower.
 *
 * In debug builds (NDEBUG not defined), asserts that the pointer is non-null
 * before dereference operations. In release builds, this check is elided.
 */
#define BORROWED_PTR_ASSERT(ptr)                                               \
  assert((ptr) != nullptr && "Borrower: Dereferencing a null borrowed pointer")

namespace ropic
{
/**
 * @brief A non-owning, move-only pointer wrapper for scope-bound access.
 *
 * Prevents ownership misconceptions by being non-copyable. Move operations
 * transfer the pointer and reset the source to nullptr to prevent accidental
 * use-after-move. Debug builds assert on null dereference.
 *
 * @tparam T The pointed-to type. Must not be a reference type.
 * @warning The pointed-to object must outlive the Borrower instance.
 * @warning After move, the source Borrower becomes null. Check with operator
 * bool() before use.
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
  explicit Borrower(T* pointer) noexcept
      : _pointer(pointer)
  {
  }

  /// @brief Copy construction disabled to prevent ownership confusion.
  Borrower(Borrower const&) = delete;

  /// @brief Copy assignment disabled to prevent ownership confusion.
  auto operator=(Borrower const&) -> Borrower& = delete;

  /// @brief Move constructor. Transfers pointer and resets source to nullptr.
  Borrower(Borrower&& other) noexcept
      : _pointer(other._pointer)
  {
    other._pointer = nullptr;
  }

  /// @brief Move assignment. Transfers pointer and resets source to nullptr.
  auto operator=(Borrower&& other) noexcept -> Borrower&
  {
    if (this != &other)
    {
      _pointer = other._pointer;
      other._pointer = nullptr;
    }
    return *this;
  }

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

  /// @brief Returns a reference to the pointed-to value. Asserts non-null in
  /// debug mode.
  /// @note Equivalent to `operator*()`. Provided for explicit naming
  /// preference.
  [[nodiscard]]
  constexpr auto value() const noexcept -> T const&
  {
    BORROWED_PTR_ASSERT(_pointer);
    return *_pointer;
  }

  /// @copydoc value() const
  [[nodiscard]]
  constexpr auto value() noexcept -> T&
  {
    BORROWED_PTR_ASSERT(_pointer);
    return *_pointer;
  }
};
} // namespace ropic
