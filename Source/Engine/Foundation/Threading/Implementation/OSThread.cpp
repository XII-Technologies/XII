#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Thread.h>

// Include inline file
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/OSThread_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/OSThread_posix.h>
#else
#  error "Thread functions are not implemented on current platform"
#endif



XII_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_OSThread);
