/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_LINUX)
#  include <Foundation/Network/Transport/Implementation/NetworkQuic.h>
#  include <sched.h>
#  include <unistd.h>

xiiUInt32 xiiNetworkQuicProcessors(xiiUInt16 (&out_uiProcessors)[2])
{
  cpu_set_t affinity;
  CPU_ZERO(&affinity);
  const long iCount = ::sysconf(_SC_NPROCESSORS_ONLN);
  if (::sched_getaffinity(0, sizeof(affinity), &affinity) != 0 || iCount <= 0)
  {
    return 0;
  }
  xiiUInt32 uiCount = 0;
  for (int iCpu = 0; iCpu < iCount && iCpu < CPU_SETSIZE && uiCount < 2; ++iCpu)
  {
    if (CPU_ISSET(iCpu, &affinity))
    {
      out_uiProcessors[uiCount++] = static_cast<xiiUInt16>(iCpu);
    }
  }
  return uiCount;
}
#endif
