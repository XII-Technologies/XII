/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Logging/Log.h>

namespace xiiLogWriter
{
  /// A simple log writer that outputs all log messages to the XII ETW provider.
  class XII_FOUNDATION_DLL ETW
  {
  public:
    /// Register this at xiiLog to write all log messages to ETW.
    static void LogMessageHandler(const xiiLoggingEventData& eventData);

    /// Log Message to ETW.
    static void LogMessage(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, xiiStringView sText);
  };
} // namespace xiiLogWriter
