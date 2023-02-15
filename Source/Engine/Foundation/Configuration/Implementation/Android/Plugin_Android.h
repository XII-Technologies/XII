#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Configuration/Plugin.h>

typedef void* xiiPluginModule;

void xiiPlugin::GetPluginPaths(const char* szPluginName, xiiStringBuilder& sOriginalFile, xiiStringBuilder& sCopiedFile, xiiUInt8 uiFileCopyNumber)
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

xiiResult UnloadPluginModule(xiiPluginModule& Module, const char* szPluginFile)
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return XII_FAILURE;
}

xiiResult LoadPluginModule(const char* szFileToLoad, xiiPluginModule& Module, const char* szPluginFile)
{
  XII_ASSERT_NOT_IMPLEMENTED;
  return XII_FAILURE;
}
