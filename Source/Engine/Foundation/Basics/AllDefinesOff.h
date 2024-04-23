#pragma once

/// \file

/// \brief Used in conjunction with XII_ENABLED and XII_DISABLED for safe checks. Define something to XII_ON or XII_OFF to work with those macros.
#define XII_ON =

/// \brief Used in conjunction with XII_ENABLED and XII_DISABLED for safe checks. Define something to XII_ON or XII_OFF to work with those macros.
#define XII_OFF !

/// \brief Used in conjunction with XII_ON and XII_OFF for safe checks. Use #if XII_ENABLED(x) or #if XII_DISABLED(x) in conditional compilation.
#define XII_ENABLED(x) (1 XII_CONCAT(x, =) 1)

/// \brief Used in conjunction with XII_ON and XII_OFF for safe checks. Use #if XII_ENABLED(x) or #if XII_DISABLED(x) in conditional compilation.
#define XII_DISABLED(x) (1 XII_CONCAT(x, =) 2)

/// \brief Checks whether x AND y are both defined as XII_ON or XII_OFF. Usually used to check whether configurations overlap, to issue an error.
#define XII_IS_NOT_EXCLUSIVE(x, y) ((1 XII_CONCAT(x, =) 1) == (1 XII_CONCAT(y, =) 1))



// All the supported Platforms
#define XII_PLATFORM_WINDOWS         XII_OFF // Enabled for all Windows platforms, both UWP and desktop.
#define XII_PLATFORM_WINDOWS_UWP     XII_OFF // Enabled for UWP apps, together with XII_PLATFORM_WINDOWS.
#define XII_PLATFORM_WINDOWS_DESKTOP XII_OFF // Enabled for desktop apps, together with XII_PLATFORM_WINDOWS.
#define XII_PLATFORM_OSX             XII_OFF
#define XII_PLATFORM_LINUX           XII_OFF
#define XII_PLATFORM_IOS             XII_OFF
#define XII_PLATFORM_ANDROID         XII_OFF

// Different Bit OSes
#define XII_PLATFORM_32BIT XII_OFF
#define XII_PLATFORM_64BIT XII_OFF

// Different CPU architectures
#define XII_PLATFORM_ARCH_X86 XII_OFF
#define XII_PLATFORM_ARCH_ARM XII_OFF

// Endianess
#define XII_PLATFORM_LITTLE_ENDIAN XII_OFF
#define XII_PLATFORM_BIG_ENDIAN    XII_OFF

// Different Compilers
#define XII_COMPILER_MSVC       XII_OFF
#define XII_COMPILER_MSVC_CLANG XII_OFF // Clang front-end with MSVC CodeGen.
#define XII_COMPILER_MSVC_PURE  XII_OFF // MSVC front-end and CodeGen, no mixed compilers.
#define XII_COMPILER_CLANG      XII_OFF
#define XII_COMPILER_GCC        XII_OFF

// How to compile the engine
#define XII_COMPILE_ENGINE_AS_DLL   XII_OFF
#define XII_COMPILE_FOR_DEBUG       XII_OFF
#define XII_COMPILE_FOR_DEVELOPMENT XII_OFF

// Platform Features
#define XII_USE_POSIX_FILE_API                XII_OFF
#define XII_USE_LINUX_POSIX_EXTENSIONS        XII_OFF // Linux specific posix extensions like pipe2, dup3, etc.
#define XII_SUPPORTS_FILE_ITERATORS           XII_OFF
#define XII_SUPPORTS_FILE_STATS               XII_OFF
#define XII_SUPPORTS_DIRECTORY_WATCHER        XII_OFF
#define XII_SUPPORTS_MEMORY_MAPPED_FILE       XII_OFF
#define XII_SUPPORTS_SHARED_MEMORY            XII_OFF
#define XII_SUPPORTS_DYNAMIC_PLUGINS          XII_OFF
#define XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS XII_OFF
#define XII_SUPPORTS_CASE_INSENSITIVE_PATHS   XII_OFF
#define XII_SUPPORTS_CRASH_DUMPS              XII_OFF
#define XII_SUPPORTS_LONG_PATHS               XII_OFF

// Allocators
#define XII_USE_GUARDED_ALLOCATIONS XII_OFF
#define XII_ALLOC_TRACKING_DEFAULT  xiiAllocatorTrackingMode::DoNotTrack

// Precision
#define XII_DOUBLE_PRECISION XII_OFF

// Other Features
#define XII_USE_PROFILING XII_OFF

// Hashed String
/// \brief Ref counting on hashed strings adds the possibility to cleanup unused strings. Since ref counting has a performance overhead it is disabled
/// by default.
#define XII_HASHED_STRING_REF_COUNTING XII_OFF

// Math Debug Checks
#define XII_MATH_CHECK_FOR_NAN XII_OFF

// SIMD support
#define XII_SIMD_IMPLEMENTATION_FPU  1
#define XII_SIMD_IMPLEMENTATION_SSE  2
#define XII_SIMD_IMPLEMENTATION_AVX  3
#define XII_SIMD_IMPLEMENTATION_NEON 4

#define XII_SIMD_IMPLEMENTATION 0

// Application entry point code injection (undef and redefine in UserConfig.h if needed)
#define XII_APPLICATION_ENTRY_POINT_CODE_INJECTION
