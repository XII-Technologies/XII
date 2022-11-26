#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Time.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Time)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    xiiTime::Initialize();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Time/Implementation/Win/Time_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/Time/Implementation/OSX/Time_osx.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Time/Implementation/Posix/Time_posix.h>
#else
#  error "Time functions are not implemented on current platform"
#endif



XII_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_Time);
