#pragma once

#include <Foundation/Logging/Log.h>

namespace xiiLogWriter
{

  /// \brief A simple log writer that outputs all log messages to the XII ETW provider.
  class XII_FOUNDATION_DLL ETW
  {
  public:
    /// \brief Register this at xiiLog to write all log messages to ETW.
    static void LogMessageHandler(const xiiLoggingEventData& eventData);

    /// \brief Log Message to ETW.
    static void LogMessage(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, xiiStringView sText);
  };
} // namespace xiiLogWriter
