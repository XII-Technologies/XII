#pragma once

#include <Foundation/Basics.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/FoundationInternal.h>
#  include <Foundation/Logging/Log.h>

XII_FOUNDATION_INTERNAL_HEADER

class xiiETWProvider
{
public:
  xiiETWProvider();
  ~xiiETWProvider();

  void LogMessge(xiiLogMsgType::Enum eventType, xiiUInt8 uiIndentation, const char* szText);

  static xiiETWProvider& GetInstance();
};
#endif
