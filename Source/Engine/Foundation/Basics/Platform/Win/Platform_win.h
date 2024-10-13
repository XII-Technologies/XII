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

#if WINAPI_FAMILY != WINAPI_FAMILY_DESKTOP_APP
#  error "Unsupported windows API family. Only the WINAPI_FAMILY_DESKTOP_APP is supported currently."
#endif

#ifndef NULL
#  define NULL 0
#endif

#undef XII_PLATFORM_LITTLE_ENDIAN
#define XII_PLATFORM_LITTLE_ENDIAN XII_ON
