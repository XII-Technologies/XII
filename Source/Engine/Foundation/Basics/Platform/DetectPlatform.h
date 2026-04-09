#pragma once

#if defined(_WINDOWS) || defined(_WIN32)
#  undef XII_PLATFORM_WINDOWS
#  define XII_PLATFORM_WINDOWS XII_ON

// Further distinction between desktop, server etc. is done in Platform_win.h

#elif defined(__APPLE__) && defined(__MACH__)
#  include <TargetConditionals.h>

#  if TARGET_OS_MAC == 1
#    undef XII_PLATFORM_OSX
#    define XII_PLATFORM_OSX XII_ON
#  endif

#elif defined(__linux)

#  undef XII_PLATFORM_LINUX
#  define XII_PLATFORM_LINUX XII_ON

#else
#  error "Unknown Platform."
#endif
