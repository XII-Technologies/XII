/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Threading/TaskSystem.h>

struct xiiAssetCuratorEvent;
class xiiTask;
struct xiiAssetInfo;

/// Asset table class. Persistent cache for an asset table.
///
/// The following assumptions need to be true for this cache to work:
/// 1. xiiAssetDocumentManager::AddEntriesToAssetTable does never change over time
/// 2. xiiAssetDocumentManager::GetAssetTableEntry never changes over the lifetime of an asset.
struct xiiAssetTable
{
  struct ManagerResource
  {
    xiiString m_sPath;
    xiiString m_sType;
  };

  xiiString                          m_sDataDir;
  xiiString                          m_sTargetFile;
  const xiiPlatformProfile*          m_pProfile = nullptr;
  bool                               m_bDirty   = true;
  bool                               m_bReset   = true;
  xiiMap<xiiString, ManagerResource> m_GuidToManagerResource;
  xiiMap<xiiString, xiiString>       m_GuidToPath;

  xiiResult WriteAssetTable();
  void      Remove(const xiiSubAsset& subAsset);
  void      Update(const xiiSubAsset& subAsset);
  void      AddManagerResource(xiiStringView sGuid, xiiStringView sPath, xiiStringView sType);
};

/// Keeps track of all asset tables and their state as well as reloading modified resources.
class XII_EDITORFRAMEWORK_DLL xiiAssetTableWriter
{
public:
  xiiAssetTableWriter(const xiiApplicationFileSystemConfig& fileSystemConfig);
  ~xiiAssetTableWriter();

  /// Needs to be called every frame. Handles update delays to allow compacting multiple changes.
  void MainThreadTick();

  /// Marks an asset that needs to be reloaded in the engine process.
  /// The requests are batched and sent out via MainThreadTick.
  void NeedsReloadResource(const xiiUuid& assetGuid);

  /// Writes the asset table for each data dir for the given asset profile.
  xiiResult WriteAssetTables(const xiiPlatformProfile* pAssetProfile, bool bForce);

private:
  void           AssetCuratorEvents(const xiiAssetCuratorEvent& e);
  xiiAssetTable* GetAssetTable(xiiUInt32 uiDataDirIndex, const xiiPlatformProfile* pAssetProfile);
  xiiUInt32      FindDataDir(const xiiSubAsset& asset);

private:
  struct ReloadResource
  {
    xiiUInt32 m_uiDataDirIndex;
    xiiString m_sResource;
    xiiString m_sType;
  };

private:
  xiiApplicationFileSystemConfig m_FileSystemConfig;
  xiiDynamicArray<xiiString>     m_DataDirRoots;

  mutable xiiCuratorMutex                                                  m_AssetTableMutex;
  bool                                                                     m_bTablesDirty           = true;
  bool                                                                     m_bNeedToReloadResources = false;
  xiiTime                                                                  m_NextTableFlush;
  xiiDynamicArray<ReloadResource>                                          m_ReloadResources;
  xiiDeque<xiiMap<const xiiPlatformProfile*, xiiUniquePtr<xiiAssetTable>>> m_DataDirToAssetTables;
};
