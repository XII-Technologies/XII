/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

class XII_FOUNDATION_DLL xiiProfilingUtils
{
public:
  /// Captures profiling data via xiiProfilingSystem::Capture and saves it to the giben file location.
  static xiiResult SaveProfilingCapture(xiiStringView sCapturePath);

  /// Reads two profiling captures and merges them into one.
  static xiiResult MergeProfilingCaptures(xiiStringView sCapturePath1, xiiStringView sCapturePath2, xiiStringView sMergedCapturePath);
};
