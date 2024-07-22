#pragma once

#if XII_DISABLED(XII_PLATFORM_OSX)
#  error "This header should only be included on OSX"
#endif

#include <cstdio>
#include <pthread.h>
#include <sys/malloc.h>
#include <sys/time.h>

// unset common macros
#undef min
#undef max

#undef XII_PLATFORM_LITTLE_ENDIAN
#define XII_PLATFORM_LITTLE_ENDIAN XII_ON
