#pragma once

/// \file

/// If set to 1, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef XII_USE_POSIX_FILE_API
#define XII_USE_POSIX_FILE_API XII_OFF

/// Iterating through the file system is supported
#undef XII_SUPPORTS_FILE_ITERATORS
#define XII_SUPPORTS_FILE_ITERATORS XII_ON

/// Getting the stats of a file (modification times etc.) is supported.
#undef XII_SUPPORTS_FILE_STATS
#define XII_SUPPORTS_FILE_STATS XII_ON

/// Directory watcher is supported on non uwp platforms.
#undef XII_SUPPORTS_DIRECTORY_WATCHER
#define XII_SUPPORTS_DIRECTORY_WATCHER XII_ON

/// Memory mapping a file is supported.
#undef XII_SUPPORTS_MEMORY_MAPPED_FILE
#define XII_SUPPORTS_MEMORY_MAPPED_FILE XII_ON

/// Shared memory IPC is supported.
#undef XII_SUPPORTS_SHARED_MEMORY
#define XII_SUPPORTS_SHARED_MEMORY XII_ON

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef XII_SUPPORTS_DYNAMIC_PLUGINS
#define XII_SUPPORTS_DYNAMIC_PLUGINS XII_ON

/// Whether applications can access any file (not sandboxed)
#undef XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS XII_ON

/// Whether file accesses can be done through paths that do not match exact casing
#undef XII_SUPPORTS_CASE_INSENSITIVE_PATHS
#define XII_SUPPORTS_CASE_INSENSITIVE_PATHS XII_ON

/// Whether starting other processes is supported.
#undef XII_SUPPORTS_PROCESSES
#define XII_SUPPORTS_PROCESSES XII_ON

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

#undef XII_SUPPORTS_CRASH_DUMPS
#define XII_SUPPORTS_CRASH_DUMPS XII_ON

#undef XII_SUPPORTS_LONG_PATHS
#define XII_SUPPORTS_LONG_PATHS XII_ON
