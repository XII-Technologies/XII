
#include <Foundation/FoundationInternal.h>
XII_FOUNDATION_INTERNAL_HEADER

#if XII_ENABLED(XII_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Configuration/Plugin.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Logging/Log.h>
#  include <Foundation/Strings/StringBuilder.h>

typedef HMODULE xiiPluginModule;

void xiiPlugin::GetPluginPaths(const char* szPluginName, xiiStringBuilder& sOriginalFile, xiiStringBuilder& sCopiedFile, xiiUInt8 uiFileCopyNumber)
{
  auto sPluginName = xiiStringView(szPluginName);

  sOriginalFile = xiiOSFile::GetApplicationDirectory();
  sOriginalFile.AppendPath(sPluginName);
  sOriginalFile.Append(".dll");

  sCopiedFile = xiiOSFile::GetApplicationDirectory();
  sCopiedFile.AppendPath(sPluginName);

  if (!xiiOSFile::ExistsFile(sOriginalFile))
  {
    sOriginalFile = xiiOSFile::GetCurrentWorkingDirectory();
    sOriginalFile.AppendPath(sPluginName);
    sOriginalFile.Append(".dll");

    sCopiedFile = xiiOSFile::GetCurrentWorkingDirectory();
    sCopiedFile.AppendPath(sPluginName);
  }

  if (uiFileCopyNumber > 0)
    sCopiedFile.AppendFormat("{0}", uiFileCopyNumber);

  sCopiedFile.Append(".loaded");
}

xiiResult UnloadPluginModule(xiiPluginModule& Module, const char* szPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

  if (FreeLibrary(Module) == FALSE)
  {
    xiiLog::Error("Could not unload plugin '{0}'. Error-Code {1}", szPluginFile, xiiArgErrorCode(GetLastError()));
    return XII_FAILURE;
  }

  Module = nullptr;
  return XII_SUCCESS;
}

xiiResult LoadPluginModule(const char* szFileToLoad, xiiPluginModule& Module, const char* szPluginFile)
{
  // reset last error code
  SetLastError(ERROR_SUCCESS);

#  if XII_ENABLED(XII_PLATFORM_WINDOWS_UWP)
  xiiStringBuilder relativePath = szFileToLoad;
  XII_SUCCEED_OR_RETURN(relativePath.MakeRelativeTo(xiiOSFile::GetApplicationDirectory()));
  Module = LoadPackagedLibrary(xiiStringWChar(relativePath).GetData(), 0);
#  else
  Module = LoadLibraryW(xiiStringWChar(szFileToLoad).GetData());
#  endif

  if (Module == nullptr)
  {
    const DWORD err = GetLastError();
    xiiLog::Error("Could not load plugin '{0}'. Error-Code {1}", szPluginFile, xiiArgErrorCode(err));

    if (err == 126)
    {
      xiiLog::Error("Please Note: This means that the plugin exists, but a DLL dependency of the plugin is missing. You probably need to copy 3rd "
                    "party DLLs next to the plugin.");
    }

    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

#else
#  error "This file should not have been included."
#endif
