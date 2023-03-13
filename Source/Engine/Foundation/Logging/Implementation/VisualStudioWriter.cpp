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

  xiiStringBuilder s;

  switch (eventData.m_EventType)
  {
    case xiiLogMsgType::BeginGroup:
      s.Format("+++++ {} ({}) +++++\n", eventData.m_sText, eventData.m_sTag);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::EndGroup:
#  if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      s.Format("----- {} ({} sec) -----\n\n", eventData.m_sText, eventData.m_fSeconds);
#  else
      s.Format("----- {} (timing info not available) -----\n\n", eventData.m_sText);
#  endif
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::ErrorMsg:
      s.Format("Error: {}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::SeriousWarningMsg:
      s.Format("Seriously: {}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::WarningMsg:
      s.Format("Warning: {}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::SuccessMsg:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::InfoMsg:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::DevMsg:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::DebugMsg:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    default:
      s.Format("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));

      xiiLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }
}

#else

void xiiLogWriter::VisualStudio::LogMessageHandler(const xiiLoggingEventData& eventData) {}

#endif


XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_VisualStudioWriter);
