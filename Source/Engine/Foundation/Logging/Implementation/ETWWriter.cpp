#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ETWWriter.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Logging/Implementation/Win/ETWProvider_win.h>

void xiiLogWriter::ETW::LogMessageHandler(const xiiLoggingEventData& eventData)
{
  if (eventData.m_EventType == xiiLogMsgType::Flush)
    return;

  xiiETWProvider::GetInstance().LogMessge(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_szText);
}

void xiiLogWriter::ETW::LogMessage(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, const char* szText)
{
  if (eventType == xiiLogMsgType::Flush)
    return;

  xiiETWProvider::GetInstance().LogMessge(eventType, uiIndentation, szText);
}

#else

void xiiLogWriter::ETW::LogMessageHandler(const xiiLoggingEventData& eventData) {}

void xiiLogWriter::ETW::LogMessage(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, const char* szText) {}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_ETWWriter);
