#pragma once

/// If set to one, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef XII_USE_POSIX_FILE_API
#define XII_USE_POSIX_FILE_API XII_ON

/// If set to one Linux posix extensions such as pipe2, dup3, etc are used.
#undef XII_USE_LINUX_POSIX_EXTENSIONS
#define XII_USE_LINUX_POSIX_EXTENSIONS XII_ON

/// Iterating through the file system is not supported
#undef XII_SUPPORTS_FILE_ITERATORS
#define XII_SUPPORTS_FILE_ITERATORS XII_OFF

/// Directory watcher is not supported
#undef XII_SUPPORTS_DIRECTORY_WATCHER
#define XII_SUPPORTS_DIRECTORY_WATCHER XII_OFF

/// Getting the stats of a file (modification times etc.) is supported.
#undef XII_SUPPORTS_FILE_STATS
#define XII_SUPPORTS_FILE_STATS XII_ON

/// Memory mapping a file is supported.
#undef XII_SUPPORTS_MEMORY_MAPPED_FILE
#define XII_SUPPORTS_MEMORY_MAPPED_FILE XII_ON

/// Shared memory IPC is not supported.
/// shm_open / shm_unlink deprecated.
/// There is an alternative in ASharedMemory_create but that is only
/// available in API 26 upwards.
/// Could be implemented via JNI which defeats the purpose of a fast IPC channel
/// or we could just use an actual file as the shared memory block.
#undef XII_SUPPORTS_SHARED_MEMORY
#define XII_SUPPORTS_SHARED_MEMORY XII_OFF

/// Whether dynamic plugins (through DLLs loaded/unloaded at runtime) are supported
#undef XII_SUPPORTS_DYNAMIC_PLUGINS
#define XII_SUPPORTS_DYNAMIC_PLUGINS XII_OFF

/// Whether applications can access any file (not sandboxed)
#undef XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#define XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS XII_OFF

/// Whether file accesses can be done through paths that do not match exact casing
#undef XII_SUPPORTS_CASE_INSENSITIVE_PATHS
#define XII_SUPPORTS_CASE_INSENSITIVE_PATHS XII_OFF

/// Whether writing to files with very long paths is supported / implemented
#undef XII_SUPPORTS_LONG_PATHS
#define XII_SUPPORTS_LONG_PATHS XII_ON

/// Whether starting other processes is supported.
#undef XII_SUPPORTS_PROCESSES
#define XII_SUPPORTS_PROCESSES XII_OFF

/// SIMD support
#undef XII_SIMD_IMPLEMENTATION
#if XII_ENABLED(XII_PLATFORM_ARCH_X86)
#  if __SSE4_1__ && __SSSE3__
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_SSE
#  else
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_FPU
#  endif
#elif XII_ENABLED(XII_PLATFORM_ARCH_ARM)
#  if XII_ENABLED(XII_PLATFORM_64BIT)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_NEON
#  else
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_FPU
#  endif
#else
#  error "Unknown architecture."
#endif

/// Use Double Precision
#undef XII_DOUBLE_PRECISION
#define XII_DOUBLE_PRECISION XII_OFF
