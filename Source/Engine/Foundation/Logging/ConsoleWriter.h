/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Logging/Log.h>

namespace xiiLogWriter
{
  /// A simple log writer that writes out log messages using printf.
  class XII_FOUNDATION_DLL Console
  {
  public:
    /// Register this at xiiLog to write all log messages to the console using printf.
    static void LogMessageHandler(const xiiLoggingEventData& eventData);

    /// Allows to indicate in what form timestamps should be added to log messages.
    static void SetTimestampMode(xiiLog::TimestampMode mode);

  private:
    static xiiLog::TimestampMode s_TimestampMode;
  };
} // namespace xiiLogWriter
