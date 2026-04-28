/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#include <dlfcn.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/Process.h>

using xiiPluginModule = void*;

bool xiiPlugin::PlatformNeedsPluginCopy()
{
  return false;
}

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

xiiResult UnloadPluginModule(xiiPluginModule& ref_pModule, xiiStringView sPluginFile)
{
  if (dlclose(ref_pModule) != 0)
  {
    xiiStringBuilder tmp;
    xiiLog::Error("Could not unload plugin '{0}'. Error {1}", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult LoadPluginModule(xiiStringView sFileToLoad, xiiPluginModule& ref_pModule, xiiStringView sPluginFile)
{
  xiiStringBuilder tmp;
  ref_pModule = dlopen(sFileToLoad.GetData(tmp), RTLD_NOW | RTLD_GLOBAL);
  if (ref_pModule == nullptr)
  {
    xiiLog::Error("Could not load plugin '{0}'. Error {1}.\nSet the environment variable LD_DEBUG=all to get more information.", sPluginFile.GetData(tmp), static_cast<const char*>(dlerror()));
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
