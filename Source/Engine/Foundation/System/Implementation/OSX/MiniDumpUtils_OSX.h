#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/System/MiniDumpUtils.h>

xiiStatus xiiMiniDumpUtils::WriteExternalProcessMiniDump(const char* szDumpFile, xiiUInt32 uiProcessID)
{
  return xiiStatus("Not implemented on OSX");
}

xiiStatus xiiMiniDumpUtils::LaunchMiniDumpTool(const char* szDumpFile)
{
  return xiiStatus("Not implemented on OSX");
}
