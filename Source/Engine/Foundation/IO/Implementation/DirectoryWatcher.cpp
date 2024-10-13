#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_SUPPORTS_DIRECTORY_WATCHER)
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)
#    include <Foundation/Platform/Implementation/Windows/DirectoryWatcher_win.h>
#  elif XII_ENABLED(XII_USE_POSIX_FILE_API)
#    include <Foundation/Platform/Implementation/Posix/DirectoryWatcher_posix.h>
#  else
#    error "Unknown Platform."
#  endif
#endif

XII_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DirectoryWatcher);
