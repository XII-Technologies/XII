#pragma once

#if XII_DISABLED(XII_PLATFORM_WINDOWS)
#  error "This header should only be included on windows platforms"
#endif

#ifdef _WIN64
#  undef XII_PLATFORM_64BIT
#  define XII_PLATFORM_64BIT XII_ON
#else
#  undef XII_PLATFORM_32BIT
#  define XII_PLATFORM_32BIT XII_ON
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <winapifamily.h>

#undef XII_PLATFORM_WINDOWS_UWP
#undef XII_PLATFORM_WINDOWS_DESKTOP

// Distinguish between Windows desktop and Windows UWP.
#if WINAPI_FAMILY == WINAPI_FAMILY_APP
#  define XII_PLATFORM_WINDOWS_UWP     XII_ON
#  define XII_PLATFORM_WINDOWS_DESKTOP XII_OFF
#else
#  define XII_PLATFORM_WINDOWS_UWP     XII_OFF
#  define XII_PLATFORM_WINDOWS_DESKTOP XII_ON
#endif

#ifndef NULL
#  define NULL 0
#endif

#undef XII_PLATFORM_LITTLE_ENDIAN
#define XII_PLATFORM_LITTLE_ENDIAN XII_ON
