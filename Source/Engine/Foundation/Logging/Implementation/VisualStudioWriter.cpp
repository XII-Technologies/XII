/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/StringConversion.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>

void xiiLogWriter::VisualStudio::LogMessageHandler(const xiiLoggingEventData& eventData)
{
  if (eventData.m_EventType == xiiLogMsgType::Flush)
    return;

#  if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  if (eventData.m_sTag.IsEqual_NoCase("beep"))
  {
    MessageBeep(0xFFFFFFFFU);
  }
#  endif

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
      s.SetFormat("+++++ {} ({}) +++++\n", eventData.m_sText, eventData.m_sTag);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::EndGroup:
#  if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
      s.SetFormat("----- {} ({} sec) -----\n\n", eventData.m_sText, eventData.m_fSeconds);
#  else
      s.SetFormat("----- {} (timing info not available) -----\n\n", eventData.m_sText);
#  endif
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::ErrorMsg:
      s.SetFormat("Error: {}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::SeriousWarningMsg:
      s.SetFormat("Seriously: {}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::WarningMsg:
      s.SetFormat("Warning: {}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::SuccessMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::InfoMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::DevMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    case xiiLogMsgType::DebugMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));
      break;

    default:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(xiiStringWChar(s));

      xiiLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }
}

#else

void xiiLogWriter::VisualStudio::LogMessageHandler(const xiiLoggingEventData& eventData) {}

#endif


XII_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_VisualStudioWriter);
