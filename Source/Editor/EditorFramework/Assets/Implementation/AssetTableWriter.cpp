#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Assets/AssetTableWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

xiiResult xiiAssetTable::WriteAssetTable()
{
  XII_PROFILE_SCOPE("WriteAssetTable");

  xiiStringBuilder sTemp;
  xiiString        sResourcePath;

  {
    for (auto& man : xiiAssetDocumentManager::GetAllDocumentManagers())
    {
      if (!man->GetDynamicRTTI()->IsDerivedFrom<xiiAssetDocumentManager>())
        continue;

      xiiAssetDocumentManager* pManager = static_cast<xiiAssetDocumentManager*>(man);

      // allow to add fully custom entries
      pManager->AddEntriesToAssetTable(m_sDataDir, m_pProfile, xiiMakeDelegate(&xiiAssetTable::AddManagerResource, this));
    }
  }

  if (m_bReset)
  {
    m_GuidToPath.Clear();
    xiiAssetCurator::xiiLockedSubAssetTable allSubAssetsLocked = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();

    for (auto it = allSubAssetsLocked->GetIterator(); it.IsValid(); ++it)
    {
      sTemp = it.Value().m_pAssetInfo->m_Path.GetAbsolutePath();

      // ignore all assets that are not located in this data directory
      if (!sTemp.IsPathBelowFolder(m_sDataDir))
        continue;

      Update(it.Value());
    }
    m_bReset = false;
  }

  // We don't write anything on a background process as the main editor process will have already written any dirty tables before sending an RPC request. We still want to engine process to reload any potential changes though and be able to check which resources to reload so the tables are kept up to date in memory.
  if (xiiQtEditorApp::GetSingleton()->IsBackgroundMode())
    return XII_SUCCESS;

  xiiDeferredFileWriter file;
  file.SetOutput(m_sTargetFile);

  auto Write = [](const xiiString& sGuid, const xiiString& sPath, xiiDeferredFileWriter& ref_file) {
    ref_file.WriteBytes(sGuid.GetData(), sGuid.GetElementCount()).IgnoreResult();
    ref_file.WriteBytes(";", 1).IgnoreResult();
    ref_file.WriteBytes(sPath.GetData(), sPath.GetElementCount()).IgnoreResult();
    ref_file.WriteBytes("\n", 1).IgnoreResult();
  };

  for (auto it = m_GuidToManagerResource.GetIterator(); it.IsValid(); ++it)
  {
    Write(it.Key(), it.Value().m_sPath, file);
  }

  for (auto it = m_GuidToPath.GetIterator(); it.IsValid(); ++it)
  {
    Write(it.Key(), it.Value(), file);
  }

  if (file.Close().Failed())
  {
    xiiLog::Error("Failed to open asset lookup table file '{0}'", m_sTargetFile);
    return XII_FAILURE;
  }

  m_bDirty = false;
  return XII_SUCCESS;
}

void xiiAssetTable::Remove(const xiiSubAsset& subAsset)
{
  xiiStringBuilder sTemp;
  xiiConversionUtils::ToString(subAsset.m_Data.m_Guid, sTemp);
  m_GuidToPath.Remove(sTemp);
  m_bDirty = true;
}

void xiiAssetTable::Update(const xiiSubAsset& subAsset)
{
  xiiStringBuilder         sTemp;
  xiiAssetDocumentManager* pManager = subAsset.m_pAssetInfo->GetManager();
  xiiString                sEntry   = pManager->GetAssetTableEntry(&subAsset, m_sDataDir, m_pProfile);

  // It is valid to write no asset table entry, if no redirection is required. This is used by decal assets for instance.
  if (!sEntry.IsEmpty())
  {
    xiiConversionUtils::ToString(subAsset.m_Data.m_Guid, sTemp);

    m_GuidToPath[sTemp] = sEntry;
  }
  m_bDirty = true;
}

void xiiAssetTable::AddManagerResource(xiiStringView sGuid, xiiStringView sPath, xiiStringView sType)
{
  m_GuidToManagerResource[sGuid] = ManagerResource{sPath, sType};
}

xiiAssetTableWriter::xiiAssetTableWriter(const xiiApplicationFileSystemConfig& fileSystemConfig)
{
  m_FileSystemConfig = fileSystemConfig;
  m_DataDirToAssetTables.SetCount(m_FileSystemConfig.m_DataDirs.GetCount());

  xiiStringBuilder sDataDirPath;

  m_DataDirRoots.Reserve(m_FileSystemConfig.m_DataDirs.GetCount());
  for (xiiUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
  {
    if (xiiFileSystem::ResolveSpecialDirectory(m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      xiiLog::Error("Failed to resolve data directory named '{}' at '{}'", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
      m_DataDirRoots.PushBack({});
    }
    else
    {
      m_DataDirRoots.PushBack(sDataDirPath);
    }
  }

  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiAssetTableWriter::AssetCuratorEvents, this));
}

xiiAssetTableWriter::~xiiAssetTableWriter()
{
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiAssetTableWriter::AssetCuratorEvents, this));
}

void xiiAssetTableWriter::MainThreadTick()
{
  // We must flush any pending table changes before triggering resource reloads.
  // If no resource reload is scheduled, we can just wait for the timer to run out to flush the changes.
  //
  if (m_bTablesDirty && (xiiTime::Now() > m_NextTableFlush || m_bNeedToReloadResources))
  {
    m_bTablesDirty = false;
    if (WriteAssetTables(xiiAssetCurator::GetSingleton()->GetActiveAssetProfile(), false).Failed())
    {
      xiiLog::Error("Failed to write asset tables");
    }
  }

  if (m_bNeedToReloadResources)
  {
    // We need to lock the curator first because that lock is hold when AssetCuratorEvents are called.
    auto lock = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
    XII_LOCK(m_AssetTableMutex);

    bool                      bReloadManagerResources = false;
    const xiiPlatformProfile* pCurrentProfile         = xiiAssetCurator::GetSingleton()->GetActiveAssetProfile();
    for (const ReloadResource& reload : m_ReloadResources)
    {
      if (xiiAssetTable* pTable = GetAssetTable(reload.m_uiDataDirIndex, pCurrentProfile))
      {
        if (pTable->m_GuidToPath.Contains(reload.m_sResource))
        {
          xiiReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID   = reload.m_sResource;
          msg2.m_sResourceType = reload.m_sType;
          xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
        else
        {
          // If an asset is not represented by a resource in the table we assume it is represented by a manager resource.
          // Currently we don't know how these relate, e.g. we don't know all "Decal" assets are represented by the "{ ProjectDecalAtlas }" resource. Therefore, we just reload all manager resources.
          bReloadManagerResources = true;
        }
      }
    }
    m_ReloadResources.Clear();

    if (bReloadManagerResources)
    {
      for (xiiUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
      {
        xiiAssetTable* pTable = GetAssetTable(i, pCurrentProfile);
        for (auto it : pTable->m_GuidToManagerResource)
        {
          xiiReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID   = it.Key();
          msg2.m_sResourceType = it.Value().m_sType;
          xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
      }
    }

    xiiSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadResources";
    xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
    m_bNeedToReloadResources = false;
  }
}

void xiiAssetTableWriter::NeedsReloadResource(const xiiUuid& assetGuid)
{
  xiiAssetCurator::xiiLockedSubAsset asset = xiiAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (asset.isValid())
  {
    XII_LOCK(m_AssetTableMutex);
    m_bNeedToReloadResources  = true;
    xiiString        sDocType = asset->m_Data.m_sSubAssetsDocumentTypeName.GetString();
    xiiStringBuilder sGuid;
    xiiConversionUtils::ToString(assetGuid, sGuid);
    const xiiUInt32 uiDataDirIndex = FindDataDir(*asset);
    m_ReloadResources.PushBack({uiDataDirIndex, sGuid, sDocType});
  }
}

xiiResult xiiAssetTableWriter::WriteAssetTables(const xiiPlatformProfile* pAssetProfile, bool bForce)
{
  CURATOR_PROFILE("WriteAssetTables");
  XII_LOG_BLOCK("xiiAssetCurator::WriteAssetTables");
  XII_ASSERT_DEV(pAssetProfile != nullptr, "WriteAssetTables: pAssetProfile must be set.");

  xiiResult res         = XII_SUCCESS;
  bool      bAnyChanged = false;
  {
    // We need to lock the curator first because that lock is hold when AssetCuratorEvents are called.
    auto lock = xiiAssetCurator::GetSingleton()->GetKnownSubAssets();
    XII_LOCK(m_AssetTableMutex);

    xiiStringBuilder sd;

    for (xiiUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      xiiAssetTable* table = GetAssetTable(i, pAssetProfile);
      if (!table)
      {
        xiiLog::Error("WriteAssetTables: The data dir '{}' with path '{}' could not be resolved", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
        res = XII_FAILURE;
        continue;
      }

      bAnyChanged |= (table->m_bReset || table->m_bDirty);
      if (!bForce && !table->m_bDirty && !table->m_bReset)
        continue;

      if (table->WriteAssetTable().Failed())
        res = XII_FAILURE;
    }
  }

  if (bAnyChanged && pAssetProfile == xiiAssetCurator::GetSingleton()->GetActiveAssetProfile())
  {
    xiiSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadAssetLUT";
    msg.m_sPayload  = pAssetProfile->GetConfigName();
    xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  m_NextTableFlush = xiiTime::Now() + xiiTime::MakeFromSeconds(1.5);
  return res;
}

void xiiAssetTableWriter::AssetCuratorEvents(const xiiAssetCuratorEvent& e)
{
  XII_LOCK(m_AssetTableMutex);

  const xiiPlatformProfile* pProfile = xiiAssetCurator::GetSingleton()->GetActiveAssetProfile();
  switch (e.m_Type)
  {
//#TODO Are asset table entries static or do they change with the asset?
#if 0
    case xiiAssetCuratorEvent::Type::AssetUpdated:
      if (e.m_pInfo->m_pAssetInfo->m_TransformState == xiiAssetInfo::TransformState::Unknown)
        return;
      [[fallthrough]];
#endif
    case xiiAssetCuratorEvent::Type::AssetAdded:
    case xiiAssetCuratorEvent::Type::AssetMoved:
    {
      xiiUInt32 uiDataDirIndex = FindDataDir(*e.m_pInfo);
      if (xiiAssetTable* pTable = GetAssetTable(uiDataDirIndex, pProfile))
      {
        pTable->Update(*e.m_pInfo);
        m_bTablesDirty = true;
      }
    }
    break;
    case xiiAssetCuratorEvent::Type::AssetRemoved:
    {
      xiiUInt32 uiDataDirIndex = FindDataDir(*e.m_pInfo);
      if (xiiAssetTable* pTable = GetAssetTable(uiDataDirIndex, pProfile))
      {
        pTable->Remove(*e.m_pInfo);
        m_bTablesDirty = true;
      }
    }
    break;
    case xiiAssetCuratorEvent::Type::AssetListReset:
      for (xiiUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
      {
        for (auto it : m_DataDirToAssetTables[i])
        {
          it.Value()->m_bReset = true;
        }
      }
      m_bTablesDirty = true;
      break;
    case xiiAssetCuratorEvent::Type::ActivePlatformChanged:
      if (WriteAssetTables(pProfile, false).Failed())
      {
        xiiLog::Error("Failed to write asset tables");
      }
      break;
    default:
      break;
  }
}

xiiAssetTable* xiiAssetTableWriter::GetAssetTable(xiiUInt32 uiDataDirIndex, const xiiPlatformProfile* pAssetProfile)
{
  auto it = m_DataDirToAssetTables[uiDataDirIndex].Find(pAssetProfile);
  if (!it.IsValid())
  {
    if (m_DataDirRoots[uiDataDirIndex].IsEmpty())
      return nullptr;

    xiiUniquePtr<xiiAssetTable> table = XII_DEFAULT_NEW(xiiAssetTable);
    table->m_pProfile                 = pAssetProfile;
    table->m_sDataDir                 = m_DataDirRoots[uiDataDirIndex];

    xiiStringBuilder sFinalPath(m_DataDirRoots[uiDataDirIndex], "/AssetCache/", pAssetProfile->GetConfigName(), ".xiiAidlt");
    sFinalPath.MakeCleanPath();
    table->m_sTargetFile = sFinalPath;

    it = m_DataDirToAssetTables[uiDataDirIndex].Insert(pAssetProfile, std::move(table));
  }
  return it.Value().Borrow();
}

xiiUInt32 xiiAssetTableWriter::FindDataDir(const xiiSubAsset& asset)
{
  return asset.m_pAssetInfo->m_Path.GetDataDirIndex();
}
