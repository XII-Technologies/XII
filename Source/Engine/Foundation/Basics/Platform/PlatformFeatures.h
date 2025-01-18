#pragma once

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/PlatformFeatures_win.h>
#elif XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/Basics/Platform/OSX/PlatformFeatures_OSX.h>
#elif XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Basics/Platform/Linux/PlatformFeatures_Linux.h>
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/PlatformFeatures_Android.h>
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
