#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/Policies/GuardedAllocation.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/GuardedAllocation_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Platform/Implementation/Posix/GuardedAllocation_posix.h>
#else
#  error "xiiGuardedAllocation is not implemented on current platform"
#endif

XII_STATICLINK_FILE(Foundation, Foundation_Memory_Policies_GuardedAllocation);
