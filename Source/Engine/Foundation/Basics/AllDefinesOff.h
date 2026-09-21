/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

/// Used in conjunction with XII_ENABLED and XII_DISABLED for safe checks. Define something to XII_ON or XII_OFF to work with those macros.
#define XII_ON =

/// Used in conjunction with XII_ENABLED and XII_DISABLED for safe checks. Define something to XII_ON or XII_OFF to work with those macros.
#define XII_OFF !

/// Used in conjunction with XII_ON and XII_OFF for safe checks. Use #if XII_ENABLED(x) or #if XII_DISABLED(x) in conditional compilation.
#define XII_ENABLED(x) (1 XII_PP_CONCAT(x, =) 1)

/// Used in conjunction with XII_ON and XII_OFF for safe checks. Use #if XII_ENABLED(x) or #if XII_DISABLED(x) in conditional compilation.
#define XII_DISABLED(x) (1 XII_PP_CONCAT(x, =) 2)

/// Checks whether x AND y are both defined as XII_ON or XII_OFF. Usually used to check whether configurations overlap, to issue an error.
#define XII_IS_NOT_EXCLUSIVE(x, y) ((1 XII_PP_CONCAT(x, =) 1) == (1 XII_PP_CONCAT(y, =) 1))

/// Checks that exactly one of x, y and z is defined as XII_ON.
#define XII_IS_NOT_EXCLUSIVE3(x, y, z) ((XII_ENABLED(x) + XII_ENABLED(y) + XII_ENABLED(z)) != 1)


// All the supported Platforms
#define XII_PLATFORM_WINDOWS         XII_OFF // Enabled for all Windows platforms.
#define XII_PLATFORM_WINDOWS_DESKTOP XII_OFF // Enabled for Windows Desktop platforms, together with XII_PLATFORM_WINDOWS.
#define XII_PLATFORM_WINDOWS_SERVER  XII_OFF // Enabled for Windows Server platforms, together with XII_PLATFORM_WINDOWS.
#define XII_PLATFORM_OSX             XII_OFF
#define XII_PLATFORM_LINUX           XII_OFF

// Different Bit OSes
#define XII_PLATFORM_32BIT XII_OFF
#define XII_PLATFORM_64BIT XII_OFF

// Different CPU architectures
#define XII_PLATFORM_ARCH_X86 XII_OFF
#define XII_PLATFORM_ARCH_ARM XII_OFF
#define XII_PLATFORM_ARCH_WEB XII_OFF

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
#define XII_SUPPORTS_IPC                      XII_OFF

// Allocators
#define XII_USE_GUARDED_ALLOCATIONS XII_OFF
#define XII_ALLOC_TRACKING_DEFAULT  xiiAllocatorTrackingMode::DoNotTrack

// Precision
#define XII_DOUBLE_PRECISION XII_OFF

// Other Features
#define XII_USE_PROFILING         XII_OFF
#define XII_USE_STRING_VALIDATION XII_OFF

// Hashed String
/// Ref counting on hashed strings adds the possibility to cleanup unused strings. Since ref counting has a performance overhead it is disabled by default.
#define XII_HASHED_STRING_REF_COUNTING XII_OFF

// Math Debug Checks
#define XII_MATH_CHECK_FOR_NAN XII_OFF

// SIMD support
#define XII_SIMD_IMPLEMENTATION_FPU  1 // Floating-point unit.
#define XII_SIMD_IMPLEMENTATION_SSE  2 // Streaming SIMD Extensions (SSE).
#define XII_SIMD_IMPLEMENTATION_AVX  3 // Advanced Vector Extensions (AVX).
#define XII_SIMD_IMPLEMENTATION_NEON 4 // NEON (for ARM architecture).

// SSE Levels
#define XII_SSE_20 0x20 // SSE2: Introduces 128-bit SIMD integer operations, cache management instructions, and support for double-precision floating-point.
#define XII_SSE_30 0x30 // SSE3: Adds horizontal addition/subtraction operations and enhancements for multi-threaded applications.
#define XII_SSE_31 0x31 // SSE3.1: Minor updates to SSE3, with improved efficiency for some existing instructions.
#define XII_SSE_41 0x41 // SSE4.1: Introduces a significant set of new instructions, including dot products and rounded floating-point computations.
#define XII_SSE_42 0x42 // SSE4.2: Adds string and text processing instructions (e.g., CRC32) for improved performance.

// AVX Levels
#define XII_AVX_1   0x50 // AVX1: Expands SSE instructions to 256-bit registers, supports FMA (Fused Multiply-Add) and improved power efficiency.
#define XII_AVX_2   0x51 // AVX2: Adds support for 256-bit integer operations, gather instructions, and additional FMA capabilities.
#define XII_AVX_512 0x52 // AVX-512: Extends to 512-bit registers, introduces new masking features, and supports wide-range instructions for HPC (High-Performance Computing) workloads.

#define XII_SIMD_IMPLEMENTATION 0

// Application entry point code injection (undef and redefine in UserConfig.h if needed)
#define XII_APPLICATION_ENTRY_POINT_CODE_INJECTION

// Interoperability with other libraries
#define XII_INTEROP_STL_STRINGS XII_OFF
#define XII_INTEROP_STL_SPAN    XII_OFF

// Simple Directmedia Layer (SDL) support for window and input
#define XII_SUPPORTS_SDL XII_OFF
