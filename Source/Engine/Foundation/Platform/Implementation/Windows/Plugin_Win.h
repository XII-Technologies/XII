
#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Configuration/Plugin.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Strings/StringBuilder.h>

using xiiPluginModule = HMODULE;

bool xiiPlugin::PlatformNeedsPluginCopy()
{
  return true;
}

void xiiPlugin::GetPluginPaths(xiiStringView sPluginName, xiiStringBuilder& ref_sOriginalFile, xiiStringBuilder& ref_sCopiedFile, xiiUInt8 uiFileCopyNumber)
{
  ref_sOriginalFile = xiiOSFile::GetApplicationDirectory();
  ref_sOriginalFile.AppendPath(sPluginName);
  ref_sOriginalFile.Append(".dll");

  ref_sCopiedFile = xiiOSFile::GetApplicationDirectory();
  ref_sCopiedFile.AppendPath(sPluginName);

  if (!xiiOSFile::ExistsFile(ref_sOriginalFile))
  {
    ref_sOriginalFile = xiiOSFile::GetCurrentWorkingDirectory();
    ref_sOriginalFile.AppendPath(sPluginName);
    ref_sOriginalFile.Append(".dll");

    ref_sCopiedFile = xiiOSFile::GetCurrentWorkingDirectory();
    ref_sCopiedFile.AppendPath(sPluginName);
  }

  if (uiFileCopyNumber > 0)
  {
    ref_sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);
  }

  ref_sCopiedFile.Append(".loaded");
}

xiiResult UnloadPluginModule(xiiPluginModule& ref_pModule, xiiStringView sPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

  if (FreeLibrary(ref_pModule) == FALSE)
  {
    xiiLog::Error("Could not unload plugin '{0}'. Error-Code {1}", sPluginFile, xiiArgErrorCode(GetLastError()));
    return XII_FAILURE;
  }

  ref_pModule = nullptr;
  return XII_SUCCESS;
}

xiiResult LoadPluginModule(xiiStringView sFileToLoad, xiiPluginModule& ref_pModule, xiiStringView sPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

  ref_pModule = LoadLibraryW(xiiStringWChar(sFileToLoad).GetData());

  if (ref_pModule == nullptr)
  {
    const DWORD err = GetLastError();
    xiiLog::Error("Could not load plugin '{0}'. Error-Code {1}", sPluginFile, xiiArgErrorCode(err));

    if (err == 126)
    {
      xiiLog::Error("Please Note: This means that the plugin exists, but a DLL dependency of the plugin is missing. You probably need to copy 3rd party DLLs next to the plugin.");
    }
    return XII_FAILURE;
  }
  return XII_SUCCESS;
}

#else
#  error "This file should not have been included."
#endif
