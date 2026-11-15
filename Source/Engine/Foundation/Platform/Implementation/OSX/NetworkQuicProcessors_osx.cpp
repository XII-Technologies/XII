/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_OSX)
#  include <Foundation/Network/Transport/Implementation/NetworkQuic.h>

xiiUInt32 xiiNetworkQuicProcessors(xiiUInt16 (&out_uiProcessors)[2])
{
  // The pinned Darwin provider exposes a single processor partition.
  out_uiProcessors[0] = 0;
  return 1;
}
#endif
