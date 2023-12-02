#pragma once

/// \file

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef XII_USE_POSIX_FILE_API

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  define XII_USE_POSIX_FILE_API XII_ON
#else
#  define XII_USE_POSIX_FILE_API XII_OFF
#endif

/// Iterating through the file system is supported
#undef XII_SUPPORTS_FILE_ITERATORS

#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  define XII_SUPPORTS_FILE_ITERATORS XII_OFF
#else
#  define XII_SUPPORTS_FILE_ITERATORS XII_ON
#endif

/// Getting the stats of a file (modification times etc.) is supported.
#undef XII_SUPPORTS_FILE_STATS
#define XII_SUPPORTS_FILE_STATS XII_ON

/// Directory watcher is supported on non uwp platforms.
#undef XII_SUPPORTS_DIRECTORY_WATCHER
#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  define XII_SUPPORTS_DIRECTORY_WATCHER XII_OFF
#else
#  define XII_SUPPORTS_DIRECTORY_WATCHER XII_ON
#endif

/// Memory mapping a file is supported.
#undef XII_SUPPORTS_MEMORY_MAPPED_FILE
#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  define XII_SUPPORTS_MEMORY_MAPPED_FILE XII_OFF
#else
#  define XII_SUPPORTS_MEMORY_MAPPED_FILE XII_ON
#endif

/// Shared memory IPC is supported.
#undef XII_SUPPORTS_SHARED_MEMORY
#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  define XII_SUPPORTS_SHARED_MEMORY XII_OFF
#else
#  define XII_SUPPORTS_SHARED_MEMORY XII_ON
#endif

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef XII_SUPPORTS_DYNAMIC_PLUGINS
#define XII_SUPPORTS_DYNAMIC_PLUGINS XII_ON

/// Whether applications can access any file (not sandboxed)
#undef XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  define XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS XII_OFF
#else
#  define XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS XII_ON
#endif

/// Whether file accesses can be done through paths that do not match exact casing
#undef XII_SUPPORTS_CASE_INSENSITIVE_PATHS
#define XII_SUPPORTS_CASE_INSENSITIVE_PATHS XII_ON

/// Whether starting other processes is supported.
#undef XII_SUPPORTS_PROCESSES
#if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
#  define XII_SUPPORTS_PROCESSES XII_OFF
#else
#  define XII_SUPPORTS_PROCESSES XII_ON
#endif

/// SIMD support
#undef XII_SIMD_IMPLEMENTATION

#if XII_ENABLED(XII_PLATFORM_ARCH_X86)
#  if __AVX__
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_AVX
#  elif __SSE4_1__ && __SSE3__
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_SSE
#  else
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_FPU
#  endif
#elif XII_ENABLED(XII_PLATFORM_ARCH_ARM)
#  define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_FPU
#else
#  error "Unknown architecture."
#endif

/// Writing crashdumps is only supported on windows desktop
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  undef XII_SUPPORTS_CRASH_DUMPS
#  define XII_SUPPORTS_CRASH_DUMPS XII_ON
#endif

/// Support for writing to files with very long paths is not implemented for UWP
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  undef XII_SUPPORTS_LONG_PATHS
#  define XII_SUPPORTS_LONG_PATHS XII_ON
#endif
