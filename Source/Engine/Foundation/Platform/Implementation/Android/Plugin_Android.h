#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Configuration/Plugin.h>

using xiiPluginModule = void*;

bool xiiPlugin::PlatformNeedsPluginCopy()
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return false;
}

void xiiPlugin::GetPluginPaths(xiiStringView sPluginName, xiiStringBuilder& sOriginalFile, xiiStringBuilder& sCopiedFile, xiiUInt8 uiFileCopyNumber)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

xiiResult UnloadPluginModule(xiiPluginModule& Module, xiiStringView sPluginFile)
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_FAILURE;
}

xiiResult LoadPluginModule(xiiStringView sFileToLoad, xiiPluginModule& Module, xiiStringView sPluginFile)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}
