/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Network/Transport/Implementation/NetworkQuic.h>

xiiUInt32 xiiNetworkQuicProcessors(xiiUInt16 (&out_uiProcessors)[2])
{
  DWORD_PTR uiProcess = 0;
  DWORD_PTR uiSystem  = 0;
  if (!::GetProcessAffinityMask(::GetCurrentProcess(), &uiProcess, &uiSystem))
  {
    return 0;
  }
  // The provider uses group-relative processor numbers on the current group.
  xiiUInt32 uiCount = 0;
  for (xiiUInt16 uiCpu = 0; uiCpu < sizeof(uiProcess) * 8 && uiCount < 2; ++uiCpu)
  {
    if ((uiProcess & (DWORD_PTR{1} << uiCpu)) != 0)
    {
      out_uiProcessors[uiCount++] = uiCpu;
    }
  }
  return uiCount;
}
#endif
