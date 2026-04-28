/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#if XII_ENABLED(XII_PLATFORM_LINUX) && defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT)

#  include <Foundation/FoundationInternal.h>
#  include <Foundation/Logging/Log.h>

XII_FOUNDATION_INTERNAL_HEADER

class xiiETWProvider
{
public:
  xiiETWProvider();
  ~xiiETWProvider();

  void LogMessage(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, xiiStringView sText);

  static xiiETWProvider& GetInstance();
};
#endif
