/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/PlatformFeatures_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/Basics/Platform/OSX/PlatformFeatures_OSX.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Basics/Platform/Linux/PlatformFeatures_Linux.h>
#else
#  error "Undefined platform!"
#endif

#undef XII_SUPPORTS_SDL
#ifdef BUILDSYSTEM_ENABLE_SDL_SUPPORT
#  define XII_SUPPORTS_SDL XII_ON
#else
#  define XII_SUPPORTS_SDL XII_OFF
#endif

// Now check that the defines for each feature are set (either to 1 or 0, but they must be defined)

#ifndef XII_SUPPORTS_FILE_ITERATORS
#  error "XII_SUPPORTS_FILE_ITERATORS is not defined."
#endif

#ifndef XII_USE_POSIX_FILE_API
#  error "XII_USE_POSIX_FILE_API is not defined."
#endif

#ifndef XII_SUPPORTS_FILE_STATS
#  error "XII_SUPPORTS_FILE_STATS is not defined."
#endif

#ifndef XII_SUPPORTS_MEMORY_MAPPED_FILE
#  error "XII_SUPPORTS_MEMORY_MAPPED_FILE is not defined."
#endif

#ifndef XII_SUPPORTS_SHARED_MEMORY
#  error "XII_SUPPORTS_SHARED_MEMORY is not defined."
#endif

#ifndef XII_SUPPORTS_DYNAMIC_PLUGINS
#  error "XII_SUPPORTS_DYNAMIC_PLUGINS is not defined."
#endif

#ifndef XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS
#  error "XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS is not defined."
#endif

#ifndef XII_SUPPORTS_CASE_INSENSITIVE_PATHS
#  error "XII_SUPPORTS_CASE_INSENSITIVE_PATHS is not defined."
#endif

#ifndef XII_SUPPORTS_LONG_PATHS
#  error "XII_SUPPORTS_LONG_PATHS is not defined."
#endif

#if XII_IS_NOT_EXCLUSIVE(XII_PLATFORM_32BIT, XII_PLATFORM_64BIT)
#  error "Platform is not defined as 32 Bit or 64 Bit"
#endif

#if XII_IS_NOT_EXCLUSIVE(XII_PLATFORM_LITTLE_ENDIAN, XII_PLATFORM_BIG_ENDIAN)
#  error "Endianess is not correctly defined."
#endif

#ifndef XII_MATH_CHECK_FOR_NAN
#  error "XII_MATH_CHECK_FOR_NAN is not defined."
#endif

#if XII_IS_NOT_EXCLUSIVE3(XII_PLATFORM_ARCH_X86, XII_PLATFORM_ARCH_ARM, XII_PLATFORM_ARCH_WEB)
#  error "Platform architecture is not correctly defined."
#endif

#if !defined(XII_SIMD_IMPLEMENTATION) || (XII_SIMD_IMPLEMENTATION == 0)
#  error "XII_SIMD_IMPLEMENTATION is not correctly defined."
#endif

#ifndef XII_PLATFORM_NAME
#  error "XII_PLATFORM_NAME is not defined."
#endif

#ifndef XII_DOUBLE_PRECISION
#  error "XII_DOUBLE_PRECISION is not defined."
#endif
