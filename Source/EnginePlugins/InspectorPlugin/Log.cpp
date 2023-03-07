#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Logging/Log.h>

namespace xiiLogWriter
{
  /// \brief This log-writer will broadcast all messages through xiiTelemetry, such that external applications can display the log messages.
  class Telemetry
  {
  public:
    /// \brief Register this at xiiLog to broadcast all log messages through xiiTelemetry.
    static void LogMessageHandler(const xiiLoggingEventData& eventData)
    {
      xiiTelemetryMessage msg;
      msg.SetMessageID(' LOG', ' MSG');

      msg.GetWriter() << (xiiInt8)eventData.m_EventType;
      msg.GetWriter() << (xiiUInt8)eventData.m_uiIndentation;
      msg.GetWriter() << eventData.m_szTag;
      msg.GetWriter() << eventData.m_szText;

      if (eventData.m_EventType == xiiLogMsgType::EndGroup)
      {
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
        msg.GetWriter() << eventData.m_fSeconds;
#else
        msg.GetWriter() << 0.0f;
#endif
      }

      xiiTelemetry::Broadcast(xiiTelemetry::Reliable, msg);
    }
  };
} // namespace xiiLogWriter

void AddLogWriter()
{
  xiiGlobalLog::AddLogWriter(&xiiLogWriter::Telemetry::LogMessageHandler);
}

void RemoveLogWriter()
{
  xiiGlobalLog::RemoveLogWriter(&xiiLogWriter::Telemetry::LogMessageHandler);
}


XII_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Log);
