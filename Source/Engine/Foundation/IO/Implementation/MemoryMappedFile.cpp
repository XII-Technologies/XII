#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Implementation/Windows/MemoryMappedFile_win.h>
#elif XII_ENABLED(XII_USE_POSIX_FILE_API)
#  include <Foundation/Platform/Implementation/Posix/MemoryMappedFile_posix.h>
#else
#  error "Unknown Platform."
#endif

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_MemoryMappedFile);
