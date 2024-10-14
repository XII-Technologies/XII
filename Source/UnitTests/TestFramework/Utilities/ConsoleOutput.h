#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>

#if XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <android/log.h>
#endif
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Logging/ETWWriter.h>
inline void SetConsoleColorInl(WORD ui)
{
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
}
#else
inline void SetConsoleColorInl(xiiUInt8 ui) {}
#endif

inline void OutputToConsole(xiiTestOutput::Enum type, const char* szMsg)
{
  static xiiInt32 iIndentation = 0;
  static bool     bAnyError    = false;

  switch (type)
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

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  char sz[4096];
  xiiStringUtils::snprintf(sz, 4096, "%*s%s\n", iIndentation, "", szMsg);
  OutputDebugStringW(xiiStringWChar(sz).GetData());
#endif
#if XII_ENABLED(XII_PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_DEBUG, "xiiEngine", "%*s%s\n", iIndentation, "", szMsg);
#endif

  if (type >= xiiTestOutput::Error)
  {
    fflush(stdout);
  }
}
