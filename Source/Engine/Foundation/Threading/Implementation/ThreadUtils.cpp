/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ThreadUtils)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    xiiThreadUtils::Initialize();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/ThreadUtils_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Platform/Implementation/Posix/ThreadUtils_posix.h>
#else
#  error "ThreadUtils functions are not implemented on current platform"
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadUtils);
