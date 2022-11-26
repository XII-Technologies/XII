#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Semaphore.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/Semaphore_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/Semaphore_posix.h>
#else
#  error "Semaphore is not implemented on current platform"
#endif
