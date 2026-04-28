/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#if defined(__clang__) || defined(__GNUC__)

#  if defined(__x86_64__) || defined(__i386__)
#    undef XII_PLATFORM_ARCH_X86
#    define XII_PLATFORM_ARCH_X86 XII_ON
#  elif defined(__arm__) || defined(__aarch64__)
#    undef XII_PLATFORM_ARCH_ARM
#    define XII_PLATFORM_ARCH_ARM XII_ON
#  else
#    error unhandled target architecture
#  endif

#  if defined(__x86_64__) || defined(__aarch64__)
#    undef XII_PLATFORM_64BIT
#    define XII_PLATFORM_64BIT XII_ON
#  elif defined(__i386__) || defined(__arm__)
#    undef XII_PLATFORM_32BIT
#    define XII_PLATFORM_32BIT XII_ON
#  else
#    error unhandled platform bit count
#  endif

#elif defined(_MSC_VER)

#  if defined(_M_AMD64) || defined(_M_IX86)
#    undef XII_PLATFORM_ARCH_X86
#    define XII_PLATFORM_ARCH_X86 XII_ON
#  elif defined(_M_ARM) || defined(_M_ARM64)
#    undef XII_PLATFORM_ARCH_ARM
#    define XII_PLATFORM_ARCH_ARM XII_ON
#  else
#    error unhandled target architecture
#  endif

#  if defined(_M_AMD64) || defined(_M_ARM64)
#    undef XII_PLATFORM_64BIT
#    define XII_PLATFORM_64BIT XII_ON
#  elif defined(_M_IX86) || defined(_M_ARM)
#    undef XII_PLATFORM_32BIT
#    define XII_PLATFORM_32BIT XII_ON
#  else
#    error unhandled platform bit count
#  endif

#else
#  error unhandled compiler
#endif
