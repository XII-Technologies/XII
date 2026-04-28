/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <cstdio>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

// unset common macros
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

#define XII_PLATFORM_NAME "Linux"

#undef XII_PLATFORM_LINUX
#define XII_PLATFORM_LINUX XII_ON

#undef XII_PLATFORM_LITTLE_ENDIAN
#define XII_PLATFORM_LITTLE_ENDIAN XII_ON

#undef XII_PLATFORM_PATH_SEPARATOR
#define XII_PLATFORM_PATH_SEPARATOR '/'

/// If set to one, the POSIX file implementation will be used. Otherwise a platform specific implementation must be available.
#undef XII_USE_POSIX_FILE_API
#define XII_USE_POSIX_FILE_API XII_ON

/// If set to one Linux posix extensions such as pipe2, dup3, etc are used.
#undef XII_USE_LINUX_POSIX_EXTENSIONS
#define XII_USE_LINUX_POSIX_EXTENSIONS XII_ON

/// Iterating through the file system is not supported
#undef XII_SUPPORTS_FILE_ITERATORS
#define XII_SUPPORTS_FILE_ITERATORS XII_ON

/// Getting the stats of a file (modification times etc.) is supported.
#undef XII_SUPPORTS_FILE_STATS
#define XII_SUPPORTS_FILE_STATS XII_ON

/// Directory watcher is supported
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
#define XII_SUPPORTS_CASE_INSENSITIVE_PATHS XII_OFF

/// Whether writing to files with very long paths is supported / implemented
#undef XII_SUPPORTS_LONG_PATHS
#define XII_SUPPORTS_LONG_PATHS XII_ON

/// Whether starting other processes is supported.
#undef XII_SUPPORTS_PROCESSES
#define XII_SUPPORTS_PROCESSES XII_ON

/// Whether inter-process communication via pipes is supported.
#undef XII_SUPPORTS_IPC
#define XII_SUPPORTS_IPC XII_ON

/// SIMD support
#undef XII_SIMD_IMPLEMENTATION

#if XII_ENABLED(XII_PLATFORM_ARCH_X86)
#  if defined(__AVX512F__)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_AVX
#    define XII_SSE_LEVEL           XII_AVX_512
#  elif defined(__AVX2__)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_AVX
#    define XII_SSE_LEVEL           XII_AVX_2
#  elif defined(__AVX__)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_AVX
#    define XII_SSE_LEVEL           XII_AVX_1
#  elif defined(__SSE4_2__)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_SSE
#    define XII_SSE_LEVEL           XII_SSE_42
#  elif defined(__SSE4_1__)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_SSE
#    define XII_SSE_LEVEL           XII_SSE_41
#  elif defined(__SSE3__)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_SSE
#    define XII_SSE_LEVEL           XII_SSE_30
#  elif defined(__SSE2__)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_SSE
#    define XII_SSE_LEVEL           XII_SSE_20
#  else
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_FPU
#    define XII_SSE_LEVEL           0
#  endif
#elif XII_ENABLED(XII_PLATFORM_ARCH_ARM)
#  if defined(__ARM_NEON)
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_NEON
#  else
#    define XII_SIMD_IMPLEMENTATION XII_SIMD_IMPLEMENTATION_FPU
#  endif
#else
#  error "Unknown architecture."
#endif
