#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <dlfcn.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>

using xiiPluginModule = void*;

void xiiPlugin::GetPluginPaths(xiiStringView sPluginName, xiiStringBuilder& sOriginalFile, xiiStringBuilder& sCopiedFile, xiiUInt8 uiFileCopyNumber)
{
  sOriginalFile = xiiOSFile::GetApplicationDirectory();
  sOriginalFile.AppendPath(sPluginName);
  sOriginalFile.Append(".so");

  sCopiedFile = xiiOSFile::GetApplicationDirectory();
  sCopiedFile.AppendPath(sPluginName);

  if (uiFileCopyNumber > 0)
    sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  sCopiedFile.Append(".loaded");
}

xiiResult UnloadPluginModule(xiiPluginModule& Module, xiiStringView sPluginFile)
{
  if (dlclose(Module) != 0)
  {
    xiiStringBuilder tmp;
    xiiLog::Error("Could not unload plugin '{0}'. Error {1}", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult LoadPluginModule(xiiStringView sFileToLoad, xiiPluginModule& Module, xiiStringView sPluginFile)
{
  xiiStringBuilder tmp;
  Module = dlopen(sFileToLoad.GetStartPointer(), RTLD_NOW | RTLD_GLOBAL);
  if (Module == nullptr)
  {
    xiiLog::Error("Could not load plugin '{0}'. Error {1}.\nSet the environment variable LD_DEBUG=all to get more information.", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
