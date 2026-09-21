/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Logging/Log.h>

namespace xiiLogWriter
{

  /// A simple log writer that outputs all log messages to visual studios output window
  class XII_FOUNDATION_DLL VisualStudio
  {
  public:
    /// Register this at xiiLog to write all log messages to visual studios output window.
    static void LogMessageHandler(const xiiLoggingEventData& eventData);
  };
} // namespace xiiLogWriter
