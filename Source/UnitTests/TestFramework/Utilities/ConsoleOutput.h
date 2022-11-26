#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <android/log.h>
#endif
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Logging/ETWWriter.h>
inline void SetConsoleColorInl(WORD ui)
{
#  if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#  endif
}
#else
inline void SetConsoleColorInl(xiiUInt8 ui) {}
#endif

inline void OutputToConsole(xiiTestOutput::Enum Type, const char* szMsg)
{
  static xiiInt32 iIndentation = 0;
  static bool     bAnyError    = false;

  switch (Type)
  {
    case xiiTestOutput::StartOutput:
      break;
    case xiiTestOutput::BeginBlock:
      iIndentation += 2;
      break;
    case xiiTestOutput::EndBlock:
      iIndentation -= 2;
      break;
    case xiiTestOutput::Details:
      SetConsoleColorInl(0x07);
      break;
    case xiiTestOutput::ImportantInfo:
      SetConsoleColorInl(0x07);
      break;
    case xiiTestOutput::Success:
      SetConsoleColorInl(0x0A);
      break;
    case xiiTestOutput::Message:
      SetConsoleColorInl(0x0E);
      break;
    case xiiTestOutput::Warning:
      SetConsoleColorInl(0x0C);
      break;
    case xiiTestOutput::Error:
      SetConsoleColorInl(0x0C);
      bAnyError = true;
      break;
    case xiiTestOutput::Duration:
    case xiiTestOutput::ImageDiffFile:
    case xiiTestOutput::InvalidType:
    case xiiTestOutput::AllOutputTypes:
      return;

    case xiiTestOutput::FinalResult:
      if (bAnyError)
        SetConsoleColorInl(0x0C);
      else
        SetConsoleColorInl(0x0A);

      // reset it for the next test round
      bAnyError = false;
      break;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  printf("%*s%s\n", iIndentation, "", szMsg);
  SetConsoleColorInl(0x07);

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  xiiLogMsgType::Enum logType = xiiLogMsgType::None;
  switch (Type)
  {
    case xiiTestOutput::StartOutput:
    case xiiTestOutput::InvalidType:
    case xiiTestOutput::AllOutputTypes:
      logType = xiiLogMsgType::None;
      break;
    case xiiTestOutput::BeginBlock:
      logType = xiiLogMsgType::BeginGroup;
      break;
    case xiiTestOutput::EndBlock:
      logType = xiiLogMsgType::EndGroup;
      break;
    case xiiTestOutput::ImportantInfo:
    case xiiTestOutput::Details:
    case xiiTestOutput::Message:
    case xiiTestOutput::Duration:
    case xiiTestOutput::FinalResult:
      logType = xiiLogMsgType::InfoMsg;
      break;
    case xiiTestOutput::Success:
      logType = xiiLogMsgType::SuccessMsg;
      break;
    case xiiTestOutput::Warning:
      logType = xiiLogMsgType::WarningMsg;
      break;
    case xiiTestOutput::Error:
      logType = xiiLogMsgType::ErrorMsg;
      break;
    case xiiTestOutput::ImageDiffFile:
      logType = xiiLogMsgType::DevMsg;
      break;
    default:
      break;
  }
  if (logType != xiiLogMsgType::None)
  {
    xiiLogWriter::ETW::LogMessage(xiiLogMsgType::InfoMsg, iIndentation, szMsg);
  }
#endif
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  char sz[4096];
  xiiStringUtils::snprintf(sz, 4096, "%*s%s\n", iIndentation, "", szMsg);
  OutputDebugStringW(xiiStringWChar(sz).GetData());
#endif
#if XII_ENABLED(XII_PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_DEBUG, "xiiEngine", "%*s%s\n", iIndentation, "", szMsg);
#endif

  if (Type >= xiiTestOutput::Error)
  {
    fflush(stdout);
  }
}
