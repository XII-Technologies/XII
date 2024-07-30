#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Semaphore.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/Semaphore_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Platform/Implementation/Posix/Semaphore_posix.h>
#else
#  error "Semaphore is not implemented on current platform"
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Semaphore);
