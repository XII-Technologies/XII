#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Platform/Implementation/Windows/ApplicationEntryPoint_win.h>

namespace xiiApplicationDetails
{
  void SetConsoleCtrlHandler(xiiMinWindows::BOOL(XII_WINDOWS_WINAPI* consoleHandler)(xiiMinWindows::DWORD dwCtrlType))
  {
    ::SetConsoleCtrlHandler(consoleHandler, TRUE);
  }

  static xiiMutex s_shutdownMutex;

  xiiMutex& GetShutdownMutex()
  {
    return s_shutdownMutex;
  }

} // namespace xiiApplicationDetails
#endif


XII_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_Win_ApplicationEntryPoint_win);
