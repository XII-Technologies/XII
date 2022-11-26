#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/StringConversion.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

void xiiLogWriter::VisualStudio::LogMessageHandler(const xiiLoggingEventData& eventData)
{
  if (eventData.m_EventType == xiiLogMsgType::Flush)
    return;

  static xiiMutex WriterLock; // will only be created if this writer is used at all
  XII_LOCK(WriterLock);

  if (eventData.m_EventType == xiiLogMsgType::BeginGroup)
    OutputDebugStringA("\n");

  for (xiiUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    OutputDebugStringA(" ");

  char sz[4096];

  switch (eventData.m_EventType)
  {
    case xiiLogMsgType::BeginGroup:
      xiiStringUtils::snprintf(sz, 1024, "+++++ %s (%s) +++++\n", eventData.m_szText, eventData.m_szTag);
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    case xiiLogMsgType::EndGroup:
#  if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      xiiStringUtils::snprintf(sz, 1024, "----- %s (%.6f sec) -----\n\n", eventData.m_szText, eventData.m_fSeconds);
#  else
      xiiStringUtils::snprintf(sz, 1024, "----- %s (%s) -----\n\n", eventData.m_szText, "timing info not available");
#  endif
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    case xiiLogMsgType::ErrorMsg:
      xiiStringUtils::snprintf(sz, 1024, "Error: %s\n", eventData.m_szText);
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    case xiiLogMsgType::SeriousWarningMsg:
      xiiStringUtils::snprintf(sz, 1024, "Seriously: %s\n", eventData.m_szText);
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    case xiiLogMsgType::WarningMsg:
      xiiStringUtils::snprintf(sz, 1024, "Warning: %s\n", eventData.m_szText);
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    case xiiLogMsgType::SuccessMsg:
      xiiStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    case xiiLogMsgType::InfoMsg:
      xiiStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    case xiiLogMsgType::DevMsg:
      xiiStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    case xiiLogMsgType::DebugMsg:
      xiiStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
      OutputDebugStringW(xiiStringWChar(sz).GetData());
      break;

    default:
      xiiStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
      OutputDebugStringW(xiiStringWChar(sz).GetData());

      xiiLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }
}

#else

void xiiLogWriter::VisualStudio::LogMessageHandler(const xiiLoggingEventData& eventData) {}

#endif



XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_VisualStudioWriter);
