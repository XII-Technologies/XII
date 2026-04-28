/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

  sPath.AppendPath("RuntimeConfigs/DataDirectories.ddl");
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

bool xiiQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute(xiiStringBuilder& ref_sPath, bool bCheckExists) const
{
  ref_sPath.MakeCleanPath();

  if (xiiPathUtils::IsAbsolutePath(ref_sPath))
    return true;

  if (xiiPathUtils::IsRootedPath(ref_sPath))
  {
    xiiStringBuilder sAbsPath;
    if (xiiFileSystem::ResolvePath(ref_sPath, &sAbsPath, nullptr).Succeeded())
    {
      ref_sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (xiiConversionUtils::IsStringUuid(ref_sPath))
  {
    xiiUuid guid   = xiiConversionUtils::ConvertStringToUuid(ref_sPath);
    auto    pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    ref_sPath = pAsset->m_pAssetInfo->m_Path;
    return true;
  }

  xiiStringBuilder sTemp, sFolder, sDataDirName;

  const char* szEnd = ref_sPath.FindSubString("/");
  if (szEnd)
  {
    sDataDirName.SetSubString_FromTo(ref_sPath.GetData(), szEnd);
  }
  else
  {
    sDataDirName = ref_sPath;
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
    sTemp.AppendPath(ref_sPath);
    sTemp.MakeCleanPath();

    if (!bCheckExists || xiiOSFile::ExistsFile(sTemp) || xiiOSFile::ExistsDirectory(sTemp))
    {
      ref_sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool xiiQtEditorApp::MakeDataDirectoryRelativePathAbsolute(xiiStringBuilder& ref_sPath) const
{
  if (xiiPathUtils::IsAbsolutePath(ref_sPath))
    return true;

  if (xiiPathUtils::IsRootedPath(ref_sPath))
  {
    xiiStringBuilder sAbsPath;
    if (xiiFileSystem::ResolvePath(ref_sPath, &sAbsPath, nullptr).Succeeded())
    {
      ref_sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (xiiConversionUtils::IsStringUuid(ref_sPath))
  {
    xiiUuid guid   = xiiConversionUtils::ConvertStringToUuid(ref_sPath);
    auto    pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    ref_sPath = pAsset->m_pAssetInfo->m_Path;
    return true;
  }

  xiiStringBuilder sTemp;

  for (xiiUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    sTemp.AppendPath(ref_sPath);
    sTemp.MakeCleanPath();

    if (xiiOSFile::ExistsFile(sTemp) || xiiOSFile::ExistsDirectory(sTemp))
    {
      ref_sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool xiiQtEditorApp::MakeDataDirectoryRelativePathAbsolute(xiiString& ref_sPath) const
{
  xiiStringBuilder sTemp = ref_sPath;
  bool             bRes  = MakeDataDirectoryRelativePathAbsolute(sTemp);
  ref_sPath              = sTemp;
  return bRes;
}

bool xiiQtEditorApp::MakePathDataDirectoryRelative(xiiStringBuilder& ref_sPath) const
{
  xiiStringBuilder sTemp;

  for (xiiUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (ref_sPath.IsPathBelowFolder(sTemp))
    {
      ref_sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  ref_sPath.MakeRelativeTo(xiiFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool xiiQtEditorApp::MakePathDataDirectoryParentRelative(xiiStringBuilder& ref_sPath) const
{
  xiiStringBuilder sTemp;

  for (xiiUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (ref_sPath.IsPathBelowFolder(sTemp))
    {
      sTemp.PathParentDirectory();

      ref_sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  ref_sPath.MakeRelativeTo(xiiFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool xiiQtEditorApp::MakePathDataDirectoryRelative(xiiString& ref_sPath) const
{
  xiiStringBuilder sTemp = ref_sPath;
  bool             bRes  = MakePathDataDirectoryRelative(sTemp);
  ref_sPath              = sTemp;
  return bRes;
}
