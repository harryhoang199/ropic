// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#pragma once

#include <iostream>
#include <thread>

/// @brief Sanitizer experiment selector.
enum class SanType : std::uint8_t
{
  NONE, ///< No sanitizer test.
  ASAN, ///< Trigger AddressSanitizer (use-after-scope).
  LSAN, ///< Trigger LeakSanitizer (unfreed allocation).
  TSAN  ///< Trigger ThreadSanitizer (data race).
};

/// @brief Deliberately triggers a sanitizer-detectable bug for verification.
/// @param type  Which sanitizer scenario to exercise (default: NONE).
inline void experimentSanitizer(SanType type = SanType::NONE)
{
  switch (type)
  {
  case SanType::ASAN:
  {
    int* asan;
    {
      int y = 1;
      asan = &y;
    }
    std::cout << "\n*asan = " << asan;
    break;
  }

  case SanType::LSAN:
  {
    int* lsan = new int(0);
    std::cout << "\n*lsan = " << *lsan;
    break;
  }

  case SanType::TSAN:
  {
    constexpr int kLoopCount = 100;
    int tsan = 0;
    std::jthread t1(
        [&tsan]
        {
          for (int i = 0; i < kLoopCount; ++i)
            ++tsan;
        });

    std::jthread t2(
        [&tsan]
        {
          for (int i = 0; i < kLoopCount; ++i)
            --tsan;
        });
    std::cout << "\ntsan = " << tsan;
    break;
  }

  default:
    break;
  }
}