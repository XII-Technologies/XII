#pragma once

#include <Foundation/Basics.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Implementation/Windows/ApplicationEntryPoint_win.h>

#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)

#  include <Foundation/Platform/Implementation/Posix/ApplicationEntryPoint_posix.h>

#elif XII_ENABLED(XII_PLATFORM_ANDROID)

#  include <Foundation/Platform/Implementation/Android/ApplicationEntryPoint_android.h>

#else
#  error "Missing definition of platform specific entry point!"
#endif
