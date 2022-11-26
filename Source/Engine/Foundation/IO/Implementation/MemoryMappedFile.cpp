#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/IO/Implementation/Win/MemoryMappedFile_win.h>
#elif XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  include <Foundation/IO/Implementation/Win/MemoryMappedFile_uwp.h>
#elif XII_ENABLED(XII_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/MemoryMappedFile_posix.h>
#else
#  error "Unknown Platform."
#endif


XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_MemoryMappedFile);
