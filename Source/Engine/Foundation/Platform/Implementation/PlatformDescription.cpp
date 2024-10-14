#include <Foundation/FoundationPCH.h>

#include <Foundation/Platform/PlatformDescription.h>

XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiPlatformDescription);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
xiiPlatformDescription g_PlatformDescription("Windows");
#elif XII_ENABLED(XII_PLATFORM_LINUX)
xiiPlatformDescription g_PlatformDescription("Linux");
#elif XII_ENABLED(XII_PLATFORM_ANDROID)
xiiPlatformDescription g_PlatformDescription("Android");
#elif XII_ENABLED(XII_PLATFORM_OSX)
xiiPlatformDescription g_PlatformDescription("OSX");
#else
#  error "Undefined platform!"
#endif

const xiiPlatformDescription* xiiPlatformDescription::s_pThisPlatform = &g_PlatformDescription;
