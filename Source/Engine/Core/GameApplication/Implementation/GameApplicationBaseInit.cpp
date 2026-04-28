/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/WorldModuleConfig.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Platform/PlatformDescription.h>
#include <Foundation/Types/TagRegistry.h>
#include <Foundation/Utilities/CommandLineOptions.h>

xiiCommandLineOptionBool   opt_DisableConsoleOutput("app", "-disableConsoleOutput", "Disables logging to the standard console window.", false);
xiiCommandLineOptionInt    opt_TelemetryPort("app", "-TelemetryPort", "The network port over which telemetry is sent.", xiiTelemetry::s_uiPort);
xiiCommandLineOptionString opt_Profile("app", "-profile", "The platform profile to use.", "Default");

xiiString xiiGameApplicationBase::GetBaseDataDirectoryPath() const
{
  return ">sdk/Data/Base";
}

xiiString xiiGameApplicationBase::GetProjectDataDirectoryPath() const
{
  return ">project/";
}

void xiiGameApplicationBase::ExecuteInitFunctions()
{
  Init_PlatformProfile_SetPreferred();
  Init_ConfigureTelemetry();
  Init_FileSystem_SetSpecialDirs();
  Init_LoadRequiredPlugins();
  Init_ConfigureAssetManagement();
  Init_FileSystem_ConfigureDataDirs();
  Init_LoadWorldModuleConfig();
  Init_LoadProjectPlugins();
  Init_PlatformProfile_LoadForRuntime();
  Init_ConfigureInput();
  Init_ConfigureTags();
  Init_ConfigureCVars();
  Init_SetupGraphicsDevice();
  Init_SetupDefaultResources();
}

void xiiGameApplicationBase::Init_PlatformProfile_SetPreferred()
{
  if (opt_Profile.IsOptionSpecified())
  {
    m_PlatformProfile.SetConfigName(opt_Profile.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
  }
  else
  {
    m_PlatformProfile.SetConfigName(xiiPlatformDescription::GetThisPlatformDesc().GetName());

    const xiiStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".xiiProfile");

    if (!xiiFileSystem::ExistsFile(sRuntimeProfileFile))
    {
      xiiLog::Info("Platform profile '{}' doesn't exist, switching to 'Default'", m_PlatformProfile.GetConfigName());

      m_PlatformProfile.SetConfigName("Default");
    }
  }

  m_PlatformProfile.AddMissingConfigs();
}

void xiiGameApplicationBase::BaseInit_ConfigureLogging()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  xiiGlobalLog::RemoveLogWriter(m_LogToVsID);

  if (!opt_DisableConsoleOutput.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    m_LogToConsoleID = xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
  }

  m_LogToVsID = xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
#endif
}

void xiiGameApplicationBase::Init_ConfigureTelemetry()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  xiiTelemetry::s_uiPort = static_cast<xiiUInt16>(opt_TelemetryPort.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified));
  xiiTelemetry::SetServerName(GetApplicationName());
  xiiTelemetry::CreateServer();
#endif
}

void xiiGameApplicationBase::Init_FileSystem_SetSpecialDirs()
{
  xiiFileSystem::SetSpecialDirectory("project", FindProjectDirectory());
}

void xiiGameApplicationBase::Init_ConfigureAssetManagement() {}

void xiiGameApplicationBase::Init_LoadRequiredPlugins()
{
  xiiPlugin::InitializeStaticallyLinkedPlugins();

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
  xiiPlugin::LoadPlugin("XBoxControllerPlugin", xiiPluginLoadFlags::PluginIsOptional).IgnoreResult();
#endif
}

void xiiGameApplicationBase::Init_FileSystem_ConfigureDataDirs()
{
  // ">appdir/" and ">user/" are built-in special directories
  // see xiiFileSystem::ResolveSpecialDirectory

  const xiiStringBuilder sUserDataPath(">user/", GetApplicationName());

  xiiFileSystem::CreateDirectoryStructure(sUserDataPath).AssertSuccess();

  xiiString sWritableBinRoot = ">appdir/";
  xiiString sShaderCacheRoot = ">sdk/Output/";

#if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  // On platforms where this is disabled, one can usually only write to the user directory, e.g., on mobile platforms.
  sWritableBinRoot = sUserDataPath;
#endif

  xiiFileSystem::CreateDirectoryStructure(sShaderCacheRoot).IgnoreResult();

  // for absolute paths, read-only
  xiiFileSystem::AddDataDirectory("", "GameApplicationBase", ":", xiiDataDirUsage::ReadOnly).AssertSuccess();

  // ":bin/" : writing to the binary directory
  xiiFileSystem::AddDataDirectory(sWritableBinRoot, "GameApplicationBase", "bin", xiiDataDirUsage::AllowWrites).AssertSuccess();

  // ":shadercache/" for reading and writing shader files
#if XII_DISABLED(XII_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
  xiiFileSystem::AddDataDirectory(sShaderCacheRoot, "GameApplicationBase", "shadercache", xiiDataDirUsage::ReadOnly).AssertSuccess();
#else
  xiiFileSystem::AddDataDirectory(sShaderCacheRoot, "GameApplicationBase", "shadercache", xiiDataDirUsage::AllowWrites).AssertSuccess();
#endif

  // ":appdata/" for reading and writing app user data
  xiiFileSystem::AddDataDirectory(sUserDataPath, "GameApplicationBase", "appdata", xiiDataDirUsage::AllowWrites).AssertSuccess();

  // ":base/" for reading the core engine files
  xiiFileSystem::AddDataDirectory(GetBaseDataDirectoryPath(), "GameApplicationBase", "base", xiiDataDirUsage::ReadOnly).IgnoreResult();

  // ":project/" for reading the project specific files
  xiiFileSystem::AddDataDirectory(GetProjectDataDirectoryPath(), "GameApplicationBase", "project", xiiDataDirUsage::ReadOnly).IgnoreResult();

  // ":plugins/" for plugin specific data (optional, if it exists)
  {
    xiiStringBuilder sDir;
    xiiFileSystem::ResolveSpecialDirectory(">sdk/Data/Plugins", sDir).IgnoreResult();
    if (sDir.IsAbsolutePath() && xiiOSFile::ExistsDirectory(sDir))
    {
      xiiFileSystem::AddDataDirectory(">sdk/Data/Plugins", "GameApplicationBase", "plugins", xiiDataDirUsage::ReadOnly).IgnoreResult();
    }
  }

  {
    xiiApplicationFileSystemConfig appFileSystemConfig;
    appFileSystemConfig.Load();

    // get rid of duplicates that we already hard-coded above
    for (xiiUInt32 i = appFileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
    {
      const xiiString name = appFileSystemConfig.m_DataDirs[i - 1].m_sRootName;
      if (name.IsEqual_NoCase(":") || name.IsEqual_NoCase("bin") || name.IsEqual_NoCase("shadercache") || name.IsEqual_NoCase("appdata") || name.IsEqual_NoCase("base") || name.IsEqual_NoCase("project") || name.IsEqual_NoCase("plugins"))
      {
        appFileSystemConfig.m_DataDirs.RemoveAtAndCopy(i - 1);
      }
    }

    appFileSystemConfig.Apply();
  }
}

void xiiGameApplicationBase::Init_LoadWorldModuleConfig()
{
  xiiWorldModuleConfig worldModuleConfig;
  worldModuleConfig.Load();
  worldModuleConfig.Apply();
}

void xiiGameApplicationBase::Init_LoadProjectPlugins()
{
  xiiApplicationPluginConfig appPluginConfig;
  appPluginConfig.Load();
  appPluginConfig.Apply();
}

void xiiGameApplicationBase::Init_PlatformProfile_LoadForRuntime()
{
  const xiiStringBuilder sRuntimeProfileFile(":project/RuntimeConfigs/", m_PlatformProfile.GetConfigName(), ".xiiProfile");
  m_PlatformProfile.AddMissingConfigs();

  m_PlatformProfile.LoadForRuntime(sRuntimeProfileFile).IgnoreResult();
}

void xiiGameApplicationBase::Init_ConfigureInput() {}

void xiiGameApplicationBase::Init_ConfigureTags()
{
  XII_LOG_BLOCK("Reading Tags", "Tags.ddl");

  xiiStringView sFile = ":project/RuntimeConfigs/Tags.ddl";

  xiiFileReader file;
  if (file.Open(sFile).Failed())
  {
    xiiLog::Dev("'{}' does not exist", sFile);
    return;
  }

  xiiStringBuilder tmp;

  xiiOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
  {
    xiiLog::Error("Failed to parse DDL data in tags file");
    return;
  }

  const xiiOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const xiiOpenDdlReaderElement* pTags = pRoot->GetFirstChild(); pTags != nullptr; pTags = pTags->GetSibling())
  {
    if (!pTags->IsCustomType("Tag"))
      continue;

    const xiiOpenDdlReaderElement* pName = pTags->FindChildOfType(xiiOpenDdlPrimitiveType::String, "Name");

    if (!pName)
    {
      xiiLog::Error("Incomplete tag declaration!");
      continue;
    }

    tmp = pName->GetPrimitivesString()[0];
    xiiTagRegistry::GetGlobalRegistry().RegisterTag(tmp);
  }
}

void xiiGameApplicationBase::Init_ConfigureCVars()
{
  xiiCVar::SetStorageFolder(":appdata/CVars");
  xiiCVar::LoadCVars();
}

void xiiGameApplicationBase::Init_SetupDefaultResources()
{
  // continuously unload resources that are not in use anymore
  xiiResourceManager::SetAutoFreeUnused(xiiTime::MakeFromMicroseconds(100), xiiTime::MakeFromSeconds(10.0f));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void xiiGameApplicationBase::Deinit_UnloadPlugins()
{
  xiiPlugin::UnloadAllPlugins();
}

void xiiGameApplicationBase::Deinit_ShutdownLogging()
{
#if XII_DISABLED(XII_COMPILE_FOR_DEVELOPMENT)
  // during development, keep these loggers active
  xiiGlobalLog::RemoveLogWriter(m_LogToConsoleID);
  xiiGlobalLog::RemoveLogWriter(m_LogToVsID);
#endif
}

XII_STATICLINK_FILE(Core, Core_GameApplication_Implementation_GameApplicationBaseInit);
