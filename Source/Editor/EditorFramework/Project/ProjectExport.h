/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>

struct xiiPathPatternFilter;
class xiiAssetCurator;
class xiiApplicationFileSystemConfig;
class xiiPlatformProfile;
class xiiProgressRange;

//////////////////////////////////////////////////////////////////////////

struct XII_EDITORFRAMEWORK_DLL xiiProjectExport
{
  static xiiResult ExportProject(const char* szTargetDirectory, const xiiPlatformProfile* pPlatformProfile, const xiiApplicationFileSystemConfig& dataDirs);

private:
  struct DataDirectory
  {
    xiiString         m_sTargetDirPath;
    xiiString         m_sTargetDirRootName;
    xiiSet<xiiString> m_Files;
  };

  using DirectoryMapping = xiiMap<xiiString, DataDirectory>;

  static xiiResult ClearTargetFolder(const char* szAbsFolderPath);
  static xiiResult ScanFolder(xiiSet<xiiString>& out_Files, const char* szFolder, const xiiPathPatternFilter& filter, xiiAssetCurator* pCurator, xiiDynamicArray<xiiString>* pSceneFiles, const xiiPlatformProfile* pPlatformProfile);
  static xiiResult CopyFiles(const char* szSrcFolder, const char* szDstFolder, const xiiSet<xiiString>& files, xiiProgressRange* pProgressRange);
  static xiiResult GatherGeneratedAssetManagerFiles(xiiSet<xiiString>& out_Files);
  static xiiResult CreateExportFilterFile(const char* szExpectedFile, const char* szFallbackFile);
  static xiiResult ReadExportFilters(xiiPathPatternFilter& out_DataFilter, xiiPathPatternFilter& out_BinariesFilter, const xiiPlatformProfile* pPlatformProfile);
  static xiiResult CreateDataDirectoryDDL(const DirectoryMapping& mapping, const char* szTargetDirectory);
  static xiiResult GatherAssetLookupTableFiles(DirectoryMapping& mapping, const xiiApplicationFileSystemConfig& dirConfig, const xiiPlatformProfile* pPlatformProfile);
  static xiiResult ScanDataDirectories(DirectoryMapping& mapping, const xiiApplicationFileSystemConfig& dirConfig, const xiiPathPatternFilter& dataFilter, xiiDynamicArray<xiiString>* pSceneFiles, const xiiPlatformProfile* pPlatformProfile);
  static xiiResult CopyAllFiles(DirectoryMapping& mapping, const char* szTargetDirectory);
  static xiiResult GatherBinaries(DirectoryMapping& mapping, const xiiPathPatternFilter& filter);
  static xiiResult CreateLaunchConfig(const xiiDynamicArray<xiiString>& sceneFiles, const char* szTargetDirectory);
  static xiiResult GatherGeneratedAssetFiles(xiiSet<xiiString>& out_Files, const char* szProjectDirectory);
};
