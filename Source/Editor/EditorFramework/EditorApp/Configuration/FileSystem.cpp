#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

void xiiQtEditorApp::AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName, bool bWriteable)
{
  xiiStringBuilder sPath = szSdkRootRelativePath;
  sPath.MakeCleanPath();

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    if (dd.m_sDataDirSpecialPath == sPath)
    {
      dd.m_bHardCodedDependency = true;

      if (bWriteable)
        dd.m_bWritable = true;

      return;
    }
  }

  xiiApplicationFileSystemConfig::DataDirConfig cfg;
  cfg.m_sDataDirSpecialPath  = sPath;
  cfg.m_bWritable            = bWriteable;
  cfg.m_sRootName            = szRootName;
  cfg.m_bHardCodedDependency = true;

  m_FileSystemConfig.m_DataDirs.PushBack(cfg);
}

void xiiQtEditorApp::SetFileSystemConfig(const xiiApplicationFileSystemConfig& cfg)
{
  if (m_FileSystemConfig == cfg)
    return;

  m_FileSystemConfig = cfg;
  xiiQtEditorApp::GetSingleton()->AddReloadProjectRequiredReason("The data directory configuration has changed.");

  m_FileSystemConfig.CreateDataDirStubFiles().IgnoreResult();
}

void xiiQtEditorApp::SetupDataDirectories()
{
  XII_PROFILE_SCOPE("SetupDataDirectories");
  xiiFileSystem::DetectSdkRootDirectory().IgnoreResult();

  xiiStringBuilder sPath = xiiToolsProject::GetSingleton()->GetProjectDirectory();

  xiiFileSystem::SetSpecialDirectory("project", sPath);

  sPath.AppendPath("DataDirectories.ddl");
  // we cannot use the default ":project/" path here, because that data directory will only be configured a few lines below
  // so instead we use the absolute path directly
  m_FileSystemConfig.Load(sPath);

  xiiEditorAppEvent e;
  e.m_Type = xiiEditorAppEvent::Type::BeforeApplyDataDirectories;
  m_Events.Broadcast(e);

  xiiQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base", false);
  xiiQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Plugins", "plugins", false);
  xiiQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">project/", "project", true);

  // Tell the tools project that all data directories are ok to put documents in
  {
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sPath).Succeeded())
      {
        xiiToolsProject::GetSingleton()->AddAllowedDocumentRoot(sPath);
      }
    }
  }

  m_FileSystemConfig.Apply();
}

bool xiiQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute(xiiStringBuilder& sPath, bool bCheckExists) const
{
  sPath.MakeCleanPath();

  if (xiiPathUtils::IsAbsolutePath(sPath))
    return true;

  if (xiiPathUtils::IsRootedPath(sPath))
  {
    xiiStringBuilder sAbsPath;
    if (xiiFileSystem::ResolvePath(sPath, &sAbsPath, nullptr).Succeeded())
    {
      sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (xiiConversionUtils::IsStringUuid(sPath))
  {
    xiiUuid guid   = xiiConversionUtils::ConvertStringToUuid(sPath);
    auto    pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    sPath = pAsset->m_pAssetInfo->m_sAbsolutePath;
    return true;
  }

  xiiStringBuilder sTemp, sFolder, sDataDirName;

  const char* szEnd = sPath.FindSubString("/");
  if (szEnd)
  {
    sDataDirName.SetSubString_FromTo(sPath.GetData(), szEnd);
  }
  else
  {
    sDataDirName = sPath;
  }

  for (xiiUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    // only check data directories that start with the required name
    while (sTemp.EndsWith("/") || sTemp.EndsWith("\\"))
      sTemp.Shrink(0, 1);
    const xiiStringView folderName = sTemp.GetFileName();

    if (sDataDirName != folderName)
      continue;

    sTemp.PathParentDirectory(); // the secret sauce is here
    sTemp.AppendPath(sPath);
    sTemp.MakeCleanPath();

    if (!bCheckExists || xiiOSFile::ExistsFile(sTemp) || xiiOSFile::ExistsDirectory(sTemp))
    {
      sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool xiiQtEditorApp::MakeDataDirectoryRelativePathAbsolute(xiiStringBuilder& sPath) const
{
  if (xiiPathUtils::IsAbsolutePath(sPath))
    return true;

  if (xiiPathUtils::IsRootedPath(sPath))
  {
    xiiStringBuilder sAbsPath;
    if (xiiFileSystem::ResolvePath(sPath, &sAbsPath, nullptr).Succeeded())
    {
      sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (xiiConversionUtils::IsStringUuid(sPath))
  {
    xiiUuid guid   = xiiConversionUtils::ConvertStringToUuid(sPath);
    auto    pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    sPath = pAsset->m_pAssetInfo->m_sAbsolutePath;
    return true;
  }

  xiiStringBuilder sTemp;

  for (xiiUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    sTemp.AppendPath(sPath);
    sTemp.MakeCleanPath();

    if (xiiOSFile::ExistsFile(sTemp) || xiiOSFile::ExistsDirectory(sTemp))
    {
      sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool xiiQtEditorApp::MakeDataDirectoryRelativePathAbsolute(xiiString& sPath) const
{
  xiiStringBuilder sTemp = sPath;
  bool             bRes  = MakeDataDirectoryRelativePathAbsolute(sTemp);
  sPath                  = sTemp;
  return bRes;
}

bool xiiQtEditorApp::MakePathDataDirectoryRelative(xiiStringBuilder& sPath) const
{
  xiiStringBuilder sTemp;

  for (xiiUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (sPath.IsPathBelowFolder(sTemp))
    {
      sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  sPath.MakeRelativeTo(xiiFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool xiiQtEditorApp::MakePathDataDirectoryParentRelative(xiiStringBuilder& sPath) const
{
  xiiStringBuilder sTemp;

  for (xiiUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (sPath.IsPathBelowFolder(sTemp))
    {
      sTemp.PathParentDirectory();

      sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  sPath.MakeRelativeTo(xiiFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool xiiQtEditorApp::MakePathDataDirectoryRelative(xiiString& sPath) const
{
  xiiStringBuilder sTemp = sPath;
  bool             bRes  = MakePathDataDirectoryRelative(sTemp);
  sPath                  = sTemp;
  return bRes;
}
