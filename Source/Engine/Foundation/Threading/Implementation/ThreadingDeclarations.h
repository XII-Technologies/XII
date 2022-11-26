#pragma once

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/ThreadingDeclarations_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/ThreadingDeclarations_posix.h>
#else
#  error "Unknown Platform."
#endif
