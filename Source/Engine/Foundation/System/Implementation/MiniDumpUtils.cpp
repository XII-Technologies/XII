#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/MiniDumpUtils_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/System/Implementation/OSX/MiniDumpUtils_OSX.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX) || XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Posix/MiniDumpUtils_posix.h>
#else
#  error "Mini-dump functions are not implemented on current platform"
#endif



XII_STATICLINK_FILE(Foundation, Foundation_System_Implementation_MiniDumpUtils);
