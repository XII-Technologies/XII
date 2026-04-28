/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Utilities/ConversionUtils.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>

#if XII_ENABLED(XII_PLATFORM_WINDOWS) && XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
#  include <crtdbg.h>
#endif

#if XII_ENABLED(XII_COMPILER_MSVC)
void MSVC_OutOfLine_DebugBreak(...)
{
  __debugbreak();
}
#endif

bool xiiDefaultAssertHandler(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  char szTemp[1024 * 4] = "";
  xiiStringUtils::snprintf(szTemp, XII_ARRAY_SIZE(szTemp), "\n\n *** Assertion ***\n\n    Expression: \"%s\"\n    Function: \"%s\"\n    File: \"%s\"\n    Line: %u\n    Message: \"%s\"\n\n",
                           szExpression, szFunction, szSourceFile, uiLine, szAssertMsg);
  szTemp[1024 * 4 - 1] = '\0';

  xiiLog::Print(szTemp);

  if (xiiSystemInformation::IsDebuggerAttached())
    return true;

  // If no debugger is attached we append the assert to a common file so that postmortem debugging is easier.
  if (FILE* assertLogFP = fopen("xiiDefaultAssertHandlerOutput.txt", "a"))
  {
    time_t timeUTC = time(&timeUTC);
    tm*    ptm     = gmtime(&timeUTC);

    char szTimeStr[256] = {0};
    xiiStringUtils::snprintf(szTimeStr, 256, "UTC: %s", asctime(ptm));
    fputs(szTimeStr, assertLogFP);

    fputs(szTemp, assertLogFP);

    fclose(assertLogFP);
  }

  // If the environment variable "XII_SILENT_ASSERTS" is set to a value like "1", "on", "true", "enable" or "yes"
  // the assert handler will never show a GUI that may block the application from continuing to run
  // this should be set on machines that run tests which should never get stuck but rather crash asap
  bool bSilentAsserts = false;

  if (xiiEnvironmentVariableUtils::IsVariableSet("XII_SILENT_ASSERTS"))
  {
    bSilentAsserts = xiiEnvironmentVariableUtils::GetValueInt("XII_SILENT_ASSERTS", bSilentAsserts ? 1 : 0) != 0;
  }

  if (bSilentAsserts)
    return true;

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

  // Ensure the cursor is definitely shown, since the user must be able to click buttons.
  xiiInt32 iHideCursor = 1;
  while (ShowCursor(true) < 0)
    ++iHideCursor;

#  if XII_ENABLED(XII_COMPILE_FOR_DEBUG) && defined(_DEBUG)

  xiiInt32 iRes = _CrtDbgReport(_CRT_ASSERT, szSourceFile, uiLine, nullptr, "'%s'\nFunction: %s\nMessage: %s", szExpression, szFunction, szAssertMsg);

  // currently we will ALWAYS trigger the breakpoint / crash (except for when the user presses 'ignore')
  if (iRes == 0)
  {
    // when the user ignores the assert, restore the cursor show/hide state to the previous count
    for (xiiInt32 i = 0; i < iHideCursor; ++i)
      ShowCursor(false);

    return false;
  }

#  else

  MessageBoxA(nullptr, szTemp, "Assertion", MB_ICONERROR);

#  endif

#endif

  // Always do a debug-break, in release-builds this will just crash the program.
  return true;
}

static xiiAssertHandler g_AssertHandler = &xiiDefaultAssertHandler;

xiiAssertHandler xiiGetAssertHandler()
{
  return g_AssertHandler;
}

void xiiSetAssertHandler(xiiAssertHandler handler)
{
  g_AssertHandler = handler;
}

bool xiiFailedCheck(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szMsg)
{
  // always do a debug-break if no assert handler is installed
  if (g_AssertHandler == nullptr)
    return true;

  return (*g_AssertHandler)(szSourceFile, uiLine, szFunction, szExpression, szMsg);
}

bool xiiFailedCheck(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const class xiiFormatString& msg)
{
  xiiStringBuilder tmp;
  return xiiFailedCheck(szSourceFile, uiLine, szFunction, szExpression, msg.GetTextCStr(tmp));
}

XII_STATICLINK_FILE(Foundation, Foundation_Basics_Assert);
