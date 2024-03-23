#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ETWWriter.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS) || (XII_ENABLED(XII_PLATFORM_LINUX) && defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT))

#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
#    include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#    include <Foundation/Logging/Implementation/Win/ETWProvider_win.h>
#  else
#    include <Foundation/Logging/Implementation/Win/ETWProvider_linux.h>
#  endif

void xiiLogWriter::ETW::LogMessageHandler(const xiiLoggingEventData& eventData)
{
  if (eventData.m_EventType == xiiLogMsgType::Flush)
    return;

  xiiETWProvider::GetInstance().LogMessage(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_sText);
}

void xiiLogWriter::ETW::LogMessage(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, xiiStringView sText)
{
  if (eventType == xiiLogMsgType::Flush)
    return;

  xiiETWProvider::GetInstance().LogMessage(eventType, uiIndentation, sText);
}

#else

void xiiLogWriter::ETW::LogMessageHandler(const xiiLoggingEventData& eventData) {}

void xiiLogWriter::ETW::LogMessage(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, xiiStringView sText) {}

#endif

XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_ETWWriter);
