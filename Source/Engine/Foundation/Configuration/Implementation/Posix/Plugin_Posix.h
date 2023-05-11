#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <dlfcn.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>

using xiiPluginModule = void*;

void xiiPlugin::GetPluginPaths(const char* szPluginName, xiiStringBuilder& sOriginalFile, xiiStringBuilder& sCopiedFile, xiiUInt8 uiFileCopyNumber)
{
  sOriginalFile = xiiOSFile::GetApplicationDirectory();
  sOriginalFile.AppendPath(szPluginName);
  sOriginalFile.Append(".so");

  sCopiedFile = xiiOSFile::GetApplicationDirectory();
  sCopiedFile.AppendPath(szPluginName);

  if (uiFileCopyNumber > 0)
    sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  sCopiedFile.Append(".loaded");
}

xiiResult UnloadPluginModule(xiiPluginModule& Module, const char* szPluginFile)
{
  if (dlclose(Module) != 0)
  {
    xiiLog::Error("Could not unload plugin '{0}'. Error {1}", szPluginFile, static_cast<const char*>(dlerror()));
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult LoadPluginModule(const char* szFileToLoad, xiiPluginModule& Module, const char* szPluginFile)
{
  Module = dlopen(szFileToLoad, RTLD_NOW | RTLD_GLOBAL);
  if (Module == nullptr)
  {
    xiiLog::Error("Could not load plugin '{0}'. Error {1}.\nSet the environment variable LD_DEBUG=all to get more information.", szPluginFile, static_cast<const char*>(dlerror()));
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
