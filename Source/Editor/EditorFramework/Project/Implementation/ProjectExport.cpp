#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

xiiResult xiiProjectExport::ClearTargetFolder(const char* szAbsFolderPath)
{
  if (xiiOSFile::DeleteFolder(szAbsFolderPath).Failed())
  {
    xiiLog::Error("Target folder could not be removed:\n'{}'", szAbsFolderPath);
    return XII_FAILURE;
  }

  if (xiiOSFile::CreateDirectoryStructure(szAbsFolderPath).Failed())
  {
    xiiLog::Error("Target folder could not be created:\n'{}'", szAbsFolderPath);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::ScanFolder(xiiSet<xiiString>& out_Files, const char* szFolder, const xiiPathPatternFilter& filter, xiiAssetCurator* pCurator, xiiDynamicArray<xiiString>* pSceneFiles, const xiiPlatformProfile* pPlatformProfile)
{
  xiiStringBuilder sRootFolder = szFolder;
  sRootFolder.Trim("/\\");

  const xiiUInt32 uiRootFolderLength = sRootFolder.GetElementCount();

  xiiStringBuilder sAbsFilePath, sRelFilePath;

  xiiFileSystemIterator it;
  for (it.StartSearch(sRootFolder, xiiFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
  {
    if (xiiProgress::GetGlobalProgressbar()->WasCanceled())
    {
      xiiLog::Warning("Folder scanning canceled by user");
      return XII_FAILURE;
    }

    it.GetStats().GetFullPath(sAbsFilePath);

    sRelFilePath = sAbsFilePath;
    sRelFilePath.Shrink(uiRootFolderLength, 0); // keep the slash at the front -> useful for the pattern filter

    if (!filter.PassesFilters(sRelFilePath))
    {
      if (it.GetStats().m_bIsDirectory)
        it.SkipFolder();
      else
        it.Next();

      continue;
    }

    if (it.GetStats().m_bIsDirectory)
    {
      it.Next();
      continue;
    }

    if (pCurator)
    {
      auto asset = pCurator->FindSubAsset(sAbsFilePath);

      if (asset.isValid() && asset->m_bMainAsset)
      {
        // redirect to asset output
        xiiAssetDocumentManager* pAssetMan = xiiStaticCast<xiiAssetDocumentManager*>(asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_pManager);

        sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_sAbsolutePath, nullptr, pPlatformProfile);

        sRelFilePath.Prepend("AssetCache/");
        out_Files.Insert(sRelFilePath);

        if (pSceneFiles && asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName == "Scene")
        {
          pSceneFiles->PushBack(sRelFilePath);
        }

        for (const xiiString& outputTag : asset->m_pAssetInfo->m_Info->m_Outputs)
        {
          sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_sAbsolutePath, outputTag, pPlatformProfile);

          sRelFilePath.Prepend("AssetCache/");
          out_Files.Insert(sRelFilePath);
        }

        it.Next();
        continue;
      }
    }

    out_Files.Insert(sRelFilePath);
    it.Next();
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::CopyFiles(const char* szSrcFolder, const char* szDstFolder, const xiiSet<xiiString>& files, xiiProgressRange* pProgressRange)
{
  xiiLog::Info("Source folder: {}", szSrcFolder);
  xiiLog::Info("Destination folder: {}", szDstFolder);

  xiiStringBuilder sSrc, sDst;

  for (auto itFile = files.GetIterator(); itFile.IsValid(); ++itFile)
  {
    if (xiiProgress::GetGlobalProgressbar()->WasCanceled())
    {
      xiiLog::Info("File copy operation canceled by user.");
      return XII_FAILURE;
    }

    if (pProgressRange)
    {
      pProgressRange->BeginNextStep(itFile.Key());
    }

    sSrc.Set(szSrcFolder, "/", itFile.Key());
    sDst.Set(szDstFolder, "/", itFile.Key());

    if (xiiOSFile::CopyFile(sSrc, sDst).Succeeded())
    {
      xiiLog::Info(" Copied: {}", itFile.Key());
    }
    else
    {
      xiiLog::Error(" Copy failed: {}", itFile.Key());
    }
  }

  xiiLog::Success("Finished copying files to destination '{}'", szDstFolder);
  return XII_SUCCESS;
}

xiiResult xiiProjectExport::GatherGeneratedAssetManagerFiles(xiiSet<xiiString>& out_Files)
{
  xiiHybridArray<xiiString, 4> addFiles;

  for (auto pMan : xiiDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = xiiDynamicCast<xiiAssetDocumentManager*>(pMan))
    {
      pAssMan->GetAdditionalOutputs(addFiles);

      for (const auto& file : addFiles)
      {
        out_Files.Insert(file);
      }

      addFiles.Clear();
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::CreateExportFilterFile(const char* szExpectedFile, const char* szFallbackFile)
{
  if (xiiFileSystem::ExistsFile(szExpectedFile))
    return XII_SUCCESS;

  xiiStringBuilder src;
  src.Set("#include <", szFallbackFile, ">\n\n\n[EXCLUDE]\n\n// TODO: add exclude patterns\n\n\n[INCLUDE]\n\n//TODO: add include patterns\n\n\n");

  xiiFileWriter file;
  if (file.Open(szExpectedFile).Failed())
  {
    xiiLog::Error("Failed to open '{}' for writing.", szExpectedFile);
    return XII_FAILURE;
  }

  file.WriteBytes(src.GetData(), src.GetElementCount()).AssertSuccess();
  return XII_SUCCESS;
}

xiiResult xiiProjectExport::ReadExportFilters(xiiPathPatternFilter& out_DataFilter, xiiPathPatternFilter& out_BinariesFilter, const xiiPlatformProfile* pPlatformProfile)
{
  xiiStringBuilder sDefine;
  sDefine.Format("PLATFORM_PROFILE_{} 1", pPlatformProfile->GetConfigName());
  sDefine.ToUpper();

  xiiHybridArray<xiiString, 1> ppDefines;
  ppDefines.PushBack(sDefine);

  if (xiiProjectExport::CreateExportFilterFile(":project/ProjectData.xiiExportFilter", "CommonData.xiiExportFilter").Failed())
  {
    xiiLog::Error("The file 'ProjectData.xiiExportFilter' could not be created.");
    return XII_FAILURE;
  }

  if (xiiProjectExport::CreateExportFilterFile(":project/ProjectBinaries.xiiExportFilter", "CommonBinaries.xiiExportFilter").Failed())
  {
    xiiLog::Error("The file 'ProjectBinaries.xiiExportFilter' could not be created.");
    return XII_FAILURE;
  }

  if (out_DataFilter.ReadConfigFile("ProjectData.xiiExportFilter", ppDefines).Failed())
  {
    xiiLog::Error("The file 'ProjectData.xiiExportFilter' could not be read.");
    return XII_FAILURE;
  }

  if (out_BinariesFilter.ReadConfigFile("ProjectBinaries.xiiExportFilter", ppDefines).Failed())
  {
    xiiLog::Error("The file 'ProjectBinaries.xiiExportFilter' could not be read.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::CreateDataDirectoryDDL(const DirectoryMapping& mapping, const char* szTargetDirectory)
{
  xiiApplicationFileSystemConfig cfg;

  xiiStringBuilder sPath;

  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
  {
    const auto& info = itDir.Value();

    if (info.m_sTargetDirRootName == "-")
      continue;

    sPath.Set(">sdk/", info.m_sTargetDirPath);

    auto& ddc                 = cfg.m_DataDirs.ExpandAndGetRef();
    ddc.m_sDataDirSpecialPath = sPath;
    ddc.m_sRootName           = info.m_sTargetDirRootName;
  }

  sPath.Set(szTargetDirectory, "/Data/project/DataDirectories.ddl");

  if (cfg.Save(sPath).Failed())
  {
    xiiLog::Error("Failed to write DataDirectories.ddl file.");
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::GatherAssetLookupTableFiles(DirectoryMapping& mapping, const xiiApplicationFileSystemConfig& dirConfig, const xiiPlatformProfile* pPlatformProfile)
{
  xiiStringBuilder sDataDirPath;

  for (const auto& dataDir : dirConfig.m_DataDirs)
  {
    if (xiiFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      xiiLog::Error("Failed to resolve data directory path '{}'", dataDir.m_sDataDirSpecialPath);
      return XII_FAILURE;
    }

    sDataDirPath.Trim("/\\");

    xiiStringBuilder sAidltPath("AssetCache/", pPlatformProfile->GetConfigName(), ".xiiAidlt");

    mapping[sDataDirPath].m_Files.Insert(sAidltPath);
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::ScanDataDirectories(DirectoryMapping& mapping, const xiiApplicationFileSystemConfig& dirConfig, const xiiPathPatternFilter& dataFilter, xiiDynamicArray<xiiString>* pSceneFiles, const xiiPlatformProfile* pPlatformProfile)
{
  xiiProgressRange progress("Scanning data directories", dirConfig.m_DataDirs.GetCount(), true);

  xiiUInt32 uiDataDirNumber = 1;

  xiiStringBuilder sDataDirPath, sDstPath;

  for (const auto& dataDir : dirConfig.m_DataDirs)
  {
    progress.BeginNextStep(dataDir.m_sDataDirSpecialPath);

    if (xiiFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      xiiLog::Error("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath);
      return XII_FAILURE;
    }

    sDataDirPath.Trim("/\\");

    xiiProjectExport::DataDirectory& ddInfo = mapping[sDataDirPath];

    if (!dataDir.m_sRootName.IsEmpty())
    {
      sDstPath.Set("Data/", dataDir.m_sRootName);

      ddInfo.m_sTargetDirRootName = dataDir.m_sRootName;
      ddInfo.m_sTargetDirPath     = sDstPath;
    }
    else
    {
      sDstPath.Format("Data/Extra{}", uiDataDirNumber);
      ++uiDataDirNumber;

      ddInfo.m_sTargetDirPath = sDstPath;
    }

    XII_SUCCEED_OR_RETURN(xiiProjectExport::ScanFolder(ddInfo.m_Files, sDataDirPath, dataFilter, xiiAssetCurator::GetSingleton(), pSceneFiles, pPlatformProfile));
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::CopyAllFiles(DirectoryMapping& mapping, const char* szTargetDirectory)
{
  xiiUInt32 uiTotalFiles = 0;
  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
    uiTotalFiles += itDir.Value().m_Files.GetCount();

  xiiProgressRange range("Copying files", uiTotalFiles, true);

  xiiLog::Info("Copying files to target directory '{}'", szTargetDirectory);

  xiiStringBuilder sTargetFolder;

  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
  {
    sTargetFolder.Set(szTargetDirectory, "/", itDir.Value().m_sTargetDirPath);

    if (xiiProjectExport::CopyFiles(itDir.Key(), sTargetFolder, itDir.Value().m_Files, &range).Failed())
      return XII_FAILURE;
  }

  xiiLog::Success("Finished copying all files.");
  return XII_SUCCESS;
}

xiiResult xiiProjectExport::GatherBinaries(DirectoryMapping& mapping, const xiiPathPatternFilter& filter)
{
  xiiStringBuilder sAppDir;
  sAppDir = xiiOSFile::GetApplicationDirectory();
  sAppDir.MakeCleanPath();
  sAppDir.Trim("/\\");

  xiiProjectExport::DataDirectory& ddInfo = mapping[sAppDir];
  ddInfo.m_sTargetDirPath                 = "Bin";
  ddInfo.m_sTargetDirRootName             = "-"; // don't add to data dir config

  if (xiiProjectExport::ScanFolder(ddInfo.m_Files, sAppDir, filter, nullptr, nullptr, nullptr).Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::CreateLaunchConfig(const xiiDynamicArray<xiiString>& sceneFiles, const char* szTargetDirectory)
{
  for (const auto& sf : sceneFiles)
  {
    xiiStringBuilder cmd;
    cmd.Format("start Bin/Player.exe -project \"Data/project\" -scene \"{}\"", sf);

    xiiStringBuilder bat;
    bat.Format("{}/Launch {}.bat", szTargetDirectory, xiiPathUtils::GetFileName(sf));

    xiiOSFile file;
    if (file.Open(bat, xiiFileOpenMode::Write).Failed())
    {
      xiiLog::Error("Couldn't create '{}'", bat);
      return XII_FAILURE;
    }

    file.Write(cmd.GetData(), cmd.GetElementCount()).AssertSuccess();
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::GatherGeneratedAssetFiles(xiiSet<xiiString>& out_Files, const char* szProjectDirectory)
{
  xiiStringBuilder sRoot(szProjectDirectory, "/AssetCache/Generated");

  xiiPathPatternFilter filter;
  xiiSet<xiiString>    files;
  XII_SUCCEED_OR_RETURN(ScanFolder(files, sRoot, filter, nullptr, nullptr, nullptr));

  xiiStringBuilder sFilePath;

  for (const auto& file : files)
  {
    sFilePath.Set("/AssetCache/Generated", file);
    out_Files.Insert(sFilePath);
  }

  return XII_SUCCESS;
}

xiiResult xiiProjectExport::ExportProject(const char* szTargetDirectory, const xiiPlatformProfile* pPlatformProfile, const xiiApplicationFileSystemConfig& dataDirs)
{
  xiiProgressRange mainProgress("Export Project", 7, true);
  mainProgress.SetStepWeighting(0, 0.05f); // Preparing output folder
  mainProgress.SetStepWeighting(1, 0.05f); // Generating special files
  mainProgress.SetStepWeighting(2, 0.10f); // Scanning data directories
  mainProgress.SetStepWeighting(3, 0.05f); // Gathering binaries
  mainProgress.SetStepWeighting(4, 1.0f);  // Copying files
  mainProgress.SetStepWeighting(5, 0.01f); // Writing data directory config
  mainProgress.SetStepWeighting(6, 0.01f); // Finish up

  xiiStringBuilder                   sProjectRootDir;
  xiiHybridArray<xiiString, 16>      sceneFiles;
  xiiProjectExport::DirectoryMapping fileList;

  xiiPathPatternFilter dataFilter;
  xiiPathPatternFilter binariesFilter;

  // 0
  {
    mainProgress.BeginNextStep("Preparing output folder");
    XII_SUCCEED_OR_RETURN(xiiProjectExport::ClearTargetFolder(szTargetDirectory));
  }

  // 0
  {
    xiiFileSystem::ResolveSpecialDirectory(">project", sProjectRootDir).AssertSuccess();
    sProjectRootDir.Trim("/\\");

    XII_SUCCEED_OR_RETURN(xiiProjectExport::GatherAssetLookupTableFiles(fileList, dataDirs, pPlatformProfile));
    XII_SUCCEED_OR_RETURN(xiiProjectExport::ReadExportFilters(dataFilter, binariesFilter, pPlatformProfile));
    XII_SUCCEED_OR_RETURN(xiiProjectExport::GatherGeneratedAssetFiles(fileList[sProjectRootDir].m_Files, sProjectRootDir));
  }

  // 1
  {
    mainProgress.BeginNextStep("Generating special files");
    XII_SUCCEED_OR_RETURN(xiiProjectExport::GatherGeneratedAssetManagerFiles(fileList[sProjectRootDir].m_Files));
  }

  // 2
  {
    mainProgress.BeginNextStep("Scanning data directories");
    XII_SUCCEED_OR_RETURN(xiiProjectExport::ScanDataDirectories(fileList, dataDirs, dataFilter, &sceneFiles, pPlatformProfile));
  }

  // 3
  {
    mainProgress.BeginNextStep("Gathering binaries");
    XII_SUCCEED_OR_RETURN(xiiProjectExport::GatherBinaries(fileList, binariesFilter));
  }

  // 4
  {
    mainProgress.BeginNextStep("Copying files");
    XII_SUCCEED_OR_RETURN(xiiProjectExport::CopyAllFiles(fileList, szTargetDirectory));
  }

  // 5
  {
    mainProgress.BeginNextStep("Writing data directory config");
    XII_SUCCEED_OR_RETURN(xiiProjectExport::CreateDataDirectoryDDL(fileList, szTargetDirectory));
  }

  // 6
  {
    mainProgress.BeginNextStep("Finishing up");
    XII_SUCCEED_OR_RETURN(xiiProjectExport::CreateLaunchConfig(sceneFiles, szTargetDirectory));
  }

  return XII_SUCCESS;
}
