/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#endif

// <StaticLinkUtil::StartHere>
// All include's before this will be left alone and not replaced by the StaticLinkUtil
// All include's AFTER this will be removed by the StaticLinkUtil and updated by what is actually used throughout the library
