/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Implementation/Windows/ApplicationEntryPoint_win.h>

#elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)

#  include <Foundation/Platform/Implementation/Posix/ApplicationEntryPoint_posix.h>

#else
#  error "Missing definition of platform specific entry point!"
#endif
