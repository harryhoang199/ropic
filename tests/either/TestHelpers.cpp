// SPDX-License-Identifier: MIT
// Copyright (c) 2025 ropic contributors

#include "TestHelpers.hpp"

// Static member definitions
int MoveTracker::s_copyCount = 0;
int MoveTracker::s_moveCount = 0;
int ErrorDestructorTracker::s_destructorCount = 0;
int DerivedErrorDestructorTracker::s_derivedDestructorCount = 0;
std::atomic<int> LeakDetectorError::s_instanceCount{0};
