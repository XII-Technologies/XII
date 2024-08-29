#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/System/StackTracer.h>

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#    include <Foundation/Platform/Implementation/UWP/StackTracer_uwp.h>
#  else
#    include <Foundation/Platform/Implementation/Windows/StackTracer_win.h>
#  endif

#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Posix/StackTracer_posix.h>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Platform/Implementation/Android/StackTracer_android.h>
#else
#  error "StackTracer is not implemented on current platform"
#endif

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, StackTracer)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Time"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiPlugin::Events().AddEventHandler(xiiStackTracer::OnPluginEvent);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiPlugin::Events().RemoveEventHandler(xiiStackTracer::OnPluginEvent);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

void xiiStackTracer::PrintStackTrace(const xiiArrayPtr<void*>& trace, xiiStackTracer::PrintFunc printFunc)
{
  char            buffer[32];
  const xiiUInt32 uiNumTraceEntries = trace.GetCount();
  for (xiiUInt32 i = 0; i < uiNumTraceEntries; i++)
  {
    xiiStringUtils::snprintf(buffer, XII_ARRAY_SIZE(buffer), "%s%p", i == 0 ? "" : "|", trace[i]);
    printFunc(buffer);
  }
}

XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_StackTracer);
