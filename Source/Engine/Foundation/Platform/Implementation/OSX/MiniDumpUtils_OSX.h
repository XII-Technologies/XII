/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/MiniDumpUtils.h>

xiiStatus xiiMiniDumpUtils::WriteExternalProcessMiniDump(xiiStringView sDumpFile, xiiUInt32 uiProcessID)
{
  return xiiStatus("Not implemented on OSX");
}

xiiStatus xiiMiniDumpUtils::LaunchMiniDumpTool(xiiStringView sDumpFile)
{
  return xiiStatus("Not implemented on OSX");
}
