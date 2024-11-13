#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

////////////////////////////////////////////////////////////////////////
// xiiAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

xiiAssetInfo::TransformState xiiAssetCurator::HashAsset(xiiUInt64 uiSettingsHash, const xiiHybridArray<xiiString, 16>& assetTransformDeps, const xiiHybridArray<xiiString, 16>& assetThumbnailDeps, xiiSet<xiiString>& missingTransformDeps, xiiSet<xiiString>& missingThumbnailDeps, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("HashAsset");
  xiiStringBuilder             tmp;
  xiiAssetInfo::TransformState state = xiiAssetInfo::Unknown;
  {
    // hash of the main asset file
    out_AssetHash = uiSettingsHash;
    out_ThumbHash = uiSettingsHash;

    // Iterate dependencies
    for (const auto& dep : assetTransformDeps)
    {
      xiiString sPath = dep;
      if (!AddAssetHash(sPath, false, out_AssetHash, out_ThumbHash, bForce))
      {
        missingTransformDeps.Insert(sPath);
      }
    }

    for (const auto& dep : assetThumbnailDeps)
    {
      xiiString sPath = dep;
      if (!AddAssetHash(sPath, true, out_AssetHash, out_ThumbHash, bForce))
      {
        missingThumbnailDeps.Insert(sPath);
      }
    }
  }

  if (!missingThumbnailDeps.IsEmpty())
  {
    out_ThumbHash = 0;
    state         = xiiAssetInfo::MissingThumbnailDependency;
  }
  if (!missingTransformDeps.IsEmpty())
  {
    out_AssetHash = 0;
    out_ThumbHash = 0;
    state         = xiiAssetInfo::MissingTransformDependency;
  }

  return state;
}

bool xiiAssetCurator::AddAssetHash(xiiString& sPath, bool bIsReference, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, bool bForce)
{
  if (sPath.IsEmpty())
    return true;

  if (xiiConversionUtils::IsStringUuid(sPath))
  {
    const xiiUuid                guid      = xiiConversionUtils::ConvertStringToUuid(sPath);
    xiiUInt64                    assetHash = 0;
    xiiUInt64                    thumbHash = 0;
    xiiAssetInfo::TransformState state     = UpdateAssetTransformState(guid, assetHash, thumbHash, bForce);
    if (state == xiiAssetInfo::Unknown || state == xiiAssetInfo::MissingTransformDependency || state == xiiAssetInfo::MissingThumbnailDependency || state == xiiAssetInfo::CircularDependency)
    {
      xiiLog::Error("Failed to hash dependency asset '{0}'", sPath);
      return false;
    }

    // Thumbs hash is affected by both transform dependencies and references.
    out_ThumbHash += thumbHash;
    if (!bIsReference)
    {
      // References do not affect the asset hash.
      out_AssetHash += assetHash;
    }
    return true;
  }

  if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
  {
    if (sPath.EndsWith(".color"))
    {
      // TODO: detect non-file assets and skip already in dependency gather function.
      return true;
    }
    xiiLog::Error("Failed to make path absolute '{0}'", sPath);
    return false;
  }

  xiiFileStatus fileStatus;
  xiiResult     res = xiiFileSystemModel::GetSingleton()->HashFile(sPath, fileStatus);
  if (res.Failed())
  {
    return false;
  }

  // Thumbs hash is affected by both transform dependencies and references.
  out_ThumbHash += fileStatus.m_uiHash;
  if (!bIsReference)
  {
    // References do not affect the asset hash.
    out_AssetHash += fileStatus.m_uiHash;
  }
  return true;
}

static xiiResult PatchAssetGuid(xiiStringView sAbsFilePath, xiiUuid oldGuid, xiiUuid newGuid)
{
  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (xiiDocumentManager::FindDocumentTypeFromPath(sAbsFilePath, true, pTypeDesc).Failed())
    return XII_FAILURE;

  xiiUInt32 uiTries = 0;

  xiiStringBuilder sTemp;
  xiiStringBuilder sTempTarget = xiiOSFile::GetTempDataFolder();
  sTempTarget.AppendPath(xiiPathUtils::GetFileNameAndExtension(sAbsFilePath));
  sTempTarget.ChangeFileName(xiiConversionUtils::ToString(newGuid, sTemp));

  sTemp = sAbsFilePath;
  while (pTypeDesc->m_pManager->CloneDocument(sTemp, sTempTarget, newGuid).Failed())
  {
    if (uiTries >= 5)
      return XII_FAILURE;

    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(50 * (uiTries + 1)));
    uiTries++;
  }

  xiiResult res = xiiOSFile::CopyFile(sTempTarget, sAbsFilePath);
  xiiOSFile::DeleteFile(sTempTarget).IgnoreResult();
  return res;
}

xiiResult xiiAssetCurator::EnsureAssetInfoUpdated(const xiiDataDirPath& absFilePath, const xiiFileStatus& stat, bool bForce)
{
  CURATOR_PROFILE(absFilePath);

  xiiFileSystemModel* pFiles = xiiFileSystemModel::GetSingleton();

  // Read document info outside the lock
  xiiUniquePtr<xiiAssetInfo> pNewAssetInfo;
  XII_SUCCEED_OR_RETURN(ReadAssetDocumentInfo(absFilePath, stat, pNewAssetInfo));
  XII_ASSERT_DEV(pNewAssetInfo != nullptr && pNewAssetInfo->m_Info != nullptr, "Info should be valid on success.");


  XII_LOCK(m_CuratorMutex);
  const xiiUuid oldGuid = stat.m_DocumentID;
  // if it already has a valid GUID, an xiiAssetInfo object must exist
  const bool bNewAssetFile = !stat.m_DocumentID.IsValid(); // Under this current location the asset is not known.
  xiiUuid    newGuid       = pNewAssetInfo->m_Info->m_DocumentID;

  xiiAssetInfo* pCurrentAssetInfo = nullptr;
  // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
  m_KnownAssets.TryGetValue(pNewAssetInfo->m_Info->m_DocumentID, pCurrentAssetInfo);

  xiiEnum<xiiAssetExistanceState> newExistanceState = xiiAssetExistanceState::FileUnchanged;
  if (bNewAssetFile && pCurrentAssetInfo != nullptr)
  {
    xiiFileStats    fsOldLocation;
    const bool      IsSameFile           = xiiFileSystemModel::IsSameFile(pNewAssetInfo->m_Path, pCurrentAssetInfo->m_Path);
    const xiiResult statCheckOldLocation = xiiOSFile::GetFileStats(pCurrentAssetInfo->m_Path, fsOldLocation);

    if (statCheckOldLocation.Succeeded() && !IsSameFile)
    {
      // DUPLICATED
      // Unfortunately we only know about duplicates in the order in which the filesystem tells us about files
      // That means we currently always adjust the GUID of the second, third, etc. file that we look at
      // even if we might know that changing another file makes more sense
      // This works well for when the editor is running and someone copies a file.

      xiiLog::Error("Two assets have identical GUIDs: '{0}' and '{1}'", pNewAssetInfo->m_Path.GetAbsolutePath(), pCurrentAssetInfo->m_Path.GetAbsolutePath());

      const xiiUuid mod             = xiiUuid::MakeStableUuidFromString(absFilePath);
      xiiUuid       replacementGuid = pNewAssetInfo->m_Info->m_DocumentID;
      replacementGuid.CombineWithSeed(mod);

      if (PatchAssetGuid(absFilePath, pNewAssetInfo->m_Info->m_DocumentID, replacementGuid).Failed())
      {
        xiiLog::Error("Failed to adjust GUID of asset: '{0}'", absFilePath);
        pFiles->NotifyOfChange(absFilePath);
        return XII_FAILURE;
      }

      xiiLog::Warning("Adjusted GUID of asset to make it unique: '{0}'", absFilePath);

      // now let's try that again
      pFiles->NotifyOfChange(absFilePath);
      return XII_SUCCESS;
    }
    else
    {
      // MOVED
      // Notify old location to removed stale entry.
      pFiles->UnlinkDocument(pCurrentAssetInfo->m_Path).IgnoreResult();
      pFiles->NotifyOfChange(pCurrentAssetInfo->m_Path);
      newExistanceState = xiiAssetExistanceState::FileMoved;
    }
  }

  // Guid changed, different asset found, mark old as deleted and add new one.
  if (!bNewAssetFile && oldGuid != pNewAssetInfo->m_Info->m_DocumentID)
  {
    // OVERWRITTEN
    SetAssetExistanceState(*m_KnownAssets[oldGuid], xiiAssetExistanceState::FileRemoved);
    RemoveAssetTransformState(oldGuid);
    newExistanceState = xiiAssetExistanceState::FileAdded;
  }

  if (pCurrentAssetInfo)
  {
    UntrackDependencies(pCurrentAssetInfo);
    pCurrentAssetInfo->Update(pNewAssetInfo);
    // Only update if it was not already set to not overwrite, e.g. FileMoved.
    if (newExistanceState == xiiAssetExistanceState::FileUnchanged)
      newExistanceState = xiiAssetExistanceState::FileModified;
  }
  else
  {
    pCurrentAssetInfo      = pNewAssetInfo.Release();
    m_KnownAssets[newGuid] = pCurrentAssetInfo;
    newExistanceState      = xiiAssetExistanceState::FileAdded;
  }

  TrackDependencies(pCurrentAssetInfo);
  CheckForCircularDependencies(pCurrentAssetInfo).IgnoreResult();
  UpdateAssetTransformState(newGuid, xiiAssetInfo::TransformState::Unknown);
  // Don't call SetAssetExistanceState on newly created assets as their data structure is initialized in UpdateSubAssets for the first time.
  if (newExistanceState != xiiAssetExistanceState::FileAdded)
    SetAssetExistanceState(*pCurrentAssetInfo, newExistanceState);
  UpdateSubAssets(*pCurrentAssetInfo);

  InvalidateAssetTransformState(newGuid);
  pFiles->LinkDocument(absFilePath, pCurrentAssetInfo->m_Info->m_DocumentID).AssertSuccess("Failed to link document in file system model");
  return XII_SUCCESS;
}

void xiiAssetCurator::TrackDependencies(xiiAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_TransformDependencies, m_InverseTransformDeps, m_UnresolvedTransformDeps, true);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_ThumbnailDependencies, m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps, true);

  const xiiString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, "");
  auto            it          = m_InverseThumbnailDeps.FindOrAdd(sTargetFile);
  it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const xiiString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, outputIt.Key());
    it                           = m_InverseThumbnailDeps.FindOrAdd(sTargetFile2);
    it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  }

  // Depending on the order of loading, dependencies might be unresolved until the dependency itself is loaded into the curator.
  // If pAssetInfo was previously an unresolved dependency, these two calls will update the inverse dep tables now that it can be resolved.
  UpdateUnresolvedTrackedFiles(m_InverseTransformDeps, m_UnresolvedTransformDeps);
  UpdateUnresolvedTrackedFiles(m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps);
}

void xiiAssetCurator::UntrackDependencies(xiiAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_TransformDependencies, m_InverseTransformDeps, m_UnresolvedTransformDeps, false);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_ThumbnailDependencies, m_InverseThumbnailDeps, m_UnresolvedThumbnailDeps, false);

  const xiiString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, "");
  auto            it          = m_InverseThumbnailDeps.FindOrAdd(sTargetFile);
  it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const xiiString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_Path, outputIt.Key());
    it                           = m_InverseThumbnailDeps.FindOrAdd(sTargetFile2);
    it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  }
}

xiiResult xiiAssetCurator::CheckForCircularDependencies(xiiAssetInfo* pAssetInfo)
{
  xiiSet<xiiUuid> inverseHull;
  GenerateInverseTransitiveHull(pAssetInfo, inverseHull, true, true);

  xiiResult res = XII_SUCCESS;
  for (const auto& sDep : pAssetInfo->m_Info->m_TransformDependencies)
  {
    if (xiiConversionUtils::IsStringUuid(sDep))
    {
      const xiiUuid guid = xiiConversionUtils::ConvertStringToUuid(sDep);
      if (inverseHull.Contains(guid))
      {
        pAssetInfo->m_CircularDependencies.Insert(sDep);
        res = XII_FAILURE;
      }
    }
  }

  for (const auto& sDep : pAssetInfo->m_Info->m_ThumbnailDependencies)
  {
    if (xiiConversionUtils::IsStringUuid(sDep))
    {
      const xiiUuid guid = xiiConversionUtils::ConvertStringToUuid(sDep);
      if (inverseHull.Contains(guid))
      {
        pAssetInfo->m_CircularDependencies.Insert(sDep);
        res = XII_FAILURE;
      }
    }
  }
  return res;
}

void xiiAssetCurator::UpdateTrackedFiles(const xiiUuid& assetGuid, const xiiSet<xiiString>& files, xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>>& inverseTracker, xiiSet<std::tuple<xiiUuid, xiiUuid>>& unresolved, bool bAdd)
{
  for (const auto& dep : files)
  {
    xiiString sPath = dep;

    if (sPath.IsEmpty())
      continue;

    if (xiiConversionUtils::IsStringUuid(sPath))
    {
      const xiiUuid       guid  = xiiConversionUtils::ConvertStringToUuid(sPath);
      const xiiAssetInfo* pInfo = GetAssetInfo(guid);

      if (!bAdd)
      {
        unresolved.Remove(std::tuple<xiiUuid, xiiUuid>(assetGuid, guid));
        if (pInfo == nullptr)
          continue;
      }

      if (pInfo == nullptr && bAdd)
      {
        unresolved.Insert(std::tuple<xiiUuid, xiiUuid>(assetGuid, guid));
        continue;
      }

      sPath = pInfo->m_Path.GetAbsolutePath();
    }
    else
    {
      if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      {
        continue;
      }
    }
    auto it = inverseTracker.FindOrAdd(sPath);
    if (bAdd)
    {
      it.Value().PushBack(assetGuid);
    }
    else
    {
      it.Value().RemoveAndCopy(assetGuid);
    }
  }
}

void xiiAssetCurator::UpdateUnresolvedTrackedFiles(xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>>& inverseTracker, xiiSet<std::tuple<xiiUuid, xiiUuid>>& unresolved)
{
  for (auto it = unresolved.GetIterator(); it.IsValid();)
  {
    auto&          t         = *it;
    const xiiUuid& assetGuid = std::get<0>(t);
    const xiiUuid& depGuid   = std::get<1>(t);
    if (const xiiAssetInfo* pInfo = GetAssetInfo(depGuid))
    {
      xiiString sPath     = pInfo->m_Path.GetAbsolutePath();
      auto      itTracker = inverseTracker.FindOrAdd(sPath);
      itTracker.Value().PushBack(assetGuid);
      it = unresolved.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

xiiResult xiiAssetCurator::ReadAssetDocumentInfo(const xiiDataDirPath& absFilePath, const xiiFileStatus& stat, xiiUniquePtr<xiiAssetInfo>& out_assetInfo)
{
  CURATOR_PROFILE(szAbsFilePath);
  xiiFileSystemModel* pFiles = xiiFileSystemModel::GetSingleton();

  out_assetInfo         = XII_DEFAULT_NEW(xiiAssetInfo);
  out_assetInfo->m_Path = absFilePath;

  // figure out which manager should handle this asset type
  {
    const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (out_assetInfo->m_pDocumentTypeDescriptor == nullptr)
    {
      if (xiiDocumentManager::FindDocumentTypeFromPath(absFilePath, false, pTypeDesc).Failed())
      {
        XII_REPORT_FAILURE("Invalid asset setup");
      }

      out_assetInfo->m_pDocumentTypeDescriptor = static_cast<const xiiAssetDocumentTypeDescriptor*>(pTypeDesc);
    }
  }

  // Try cache first
  {
    xiiFileStatus                      cacheStat;
    xiiUniquePtr<xiiAssetDocumentInfo> docInfo;
    {
      XII_LOCK(m_CachedAssetsMutex);
      auto itFile  = m_CachedFiles.Find(absFilePath);
      auto itAsset = m_CachedAssets.Find(absFilePath);
      if (itAsset.IsValid() && itFile.IsValid())
      {
        docInfo   = std::move(itAsset.Value());
        cacheStat = itFile.Value();
        m_CachedAssets.Remove(itAsset);
        m_CachedFiles.Remove(itFile);
      }
    }

    if (docInfo && cacheStat.m_LastModified.Compare(stat.m_LastModified, xiiTimestamp::CompareMode::Identical))
    {
      out_assetInfo->m_Info = std::move(docInfo);
      return XII_SUCCESS;
    }
  }

  // try to read the asset file
  xiiStatus infoStatus;
  xiiResult res = pFiles->ReadDocument(absFilePath, [&out_assetInfo, &infoStatus](const xiiFileStatus& stat, xiiStreamReader& ref_reader) { infoStatus = out_assetInfo->GetManager()->ReadAssetDocumentInfo(out_assetInfo->m_Info, ref_reader); });

  if (infoStatus.Failed())
  {
    xiiLog::Error("Failed to read asset document info for asset file '{0}'", absFilePath);
    return XII_FAILURE;
  }

  XII_ASSERT_DEV(out_assetInfo->m_Info != nullptr, "Info should be valid on suceess.");
  return res;
}

void xiiAssetCurator::UpdateSubAssets(xiiAssetInfo& assetInfo)
{
  CURATOR_PROFILE("UpdateSubAssets");
  if (assetInfo.m_ExistanceState == xiiAssetExistanceState::FileRemoved)
    return;

  if (assetInfo.m_ExistanceState == xiiAssetExistanceState::FileAdded)
  {
    auto& mainSub                               = m_KnownSubAssets[assetInfo.m_Info->m_DocumentID];
    mainSub.m_bMainAsset                        = true;
    mainSub.m_ExistanceState                    = xiiAssetExistanceState::FileAdded;
    mainSub.m_pAssetInfo                        = &assetInfo;
    mainSub.m_Data.m_Guid                       = assetInfo.m_Info->m_DocumentID;
    mainSub.m_Data.m_sSubAssetsDocumentTypeName = assetInfo.m_Info->m_sAssetsDocumentTypeName;
  }

  {
    xiiHybridArray<xiiSubAssetData, 4> subAssets;
    {
      CURATOR_PROFILE("FillOutSubAssetList");
      assetInfo.GetManager()->FillOutSubAssetList(*assetInfo.m_Info.Borrow(), subAssets);
    }

    for (const xiiUuid& sub : assetInfo.m_SubAssets)
    {
      m_KnownSubAssets[sub].m_ExistanceState = xiiAssetExistanceState::FileRemoved;
      m_SubAssetChanged.Insert(sub);
    }

    for (const xiiSubAssetData& data : subAssets)
    {
      const bool bExisted = m_KnownSubAssets.Find(data.m_Guid).IsValid();
      XII_ASSERT_DEV(bExisted == assetInfo.m_SubAssets.Contains(data.m_Guid), "Implementation error: m_KnownSubAssets and assetInfo.m_SubAssets are out of sync.");

      xiiSubAsset sub;
      sub.m_bMainAsset     = false;
      sub.m_ExistanceState = bExisted ? xiiAssetExistanceState::FileModified : xiiAssetExistanceState::FileAdded;
      sub.m_pAssetInfo     = &assetInfo;
      sub.m_Data           = data;
      m_KnownSubAssets.Insert(data.m_Guid, sub);

      if (!bExisted)
      {
        assetInfo.m_SubAssets.Insert(sub.m_Data.m_Guid);
        m_SubAssetChanged.Insert(sub.m_Data.m_Guid);
      }
    }

    for (auto it = assetInfo.m_SubAssets.GetIterator(); it.IsValid();)
    {
      if (m_KnownSubAssets[it.Key()].m_ExistanceState == xiiAssetExistanceState::FileRemoved)
      {
        it = assetInfo.m_SubAssets.Remove(it);
      }
      else
      {
        ++it;
      }
    }
  }
}

void xiiAssetCurator::RemoveAssetTransformState(const xiiUuid& assetGuid)
{
  XII_LOCK(m_CuratorMutex);

  for (int i = 0; i < xiiAssetInfo::TransformState::COUNT; i++)
  {
    m_TransformState[i].Remove(assetGuid);
  }
  m_TransformStateStale.Remove(assetGuid);
}


void xiiAssetCurator::InvalidateAssetTransformState(const xiiUuid& assetGuid)
{
  XII_LOCK(m_CuratorMutex);

  xiiSet<xiiUuid> hull;
  {
    xiiAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    {
      GenerateInverseTransitiveHull(pAssetInfo, hull, true, true);
    }
  }

  for (const xiiUuid& guid : hull)
  {
    xiiAssetInfo* pAssetInfo = nullptr;
    if (m_KnownAssets.TryGetValue(guid, pAssetInfo))
    {
      // We do not set pAssetInfo->m_TransformState because that is user facing and
      // as after updating the state it might just be the same as before we instead add
      // it to the queue here to prevent flickering in the GUI.
      m_TransformStateStale.Insert(guid);
      // Increasing m_LastStateUpdate will ensure that asset hash/state computations
      // that are in flight will not be written back to the asset.
      pAssetInfo->m_LastStateUpdate++;
      pAssetInfo->m_AssetHash = 0;
      pAssetInfo->m_ThumbHash = 0;
    }
  }
}

void xiiAssetCurator::UpdateAssetTransformState(const xiiUuid& assetGuid, xiiAssetInfo::TransformState state)
{
  XII_LOCK(m_CuratorMutex);

  xiiAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    m_TransformStateStale.Remove(assetGuid);
    for (int i = 0; i < xiiAssetInfo::TransformState::COUNT; i++)
    {
      m_TransformState[i].Remove(assetGuid);
    }
    m_TransformState[state].Insert(assetGuid);

    const bool bStateChanged = pAssetInfo->m_TransformState != state;

    if (bStateChanged)
    {
      pAssetInfo->m_TransformState = state;
      m_SubAssetChanged.Insert(assetGuid);
      for (const auto& key : pAssetInfo->m_SubAssets)
      {
        m_SubAssetChanged.Insert(key);
      }
    }

    switch (state)
    {
      case xiiAssetInfo::TransformState::TransformError:
      {
        // Transform errors are unexpected and invalidate any previously computed
        // state of assets depending on this one.
        auto it = m_InverseTransformDeps.Find(pAssetInfo->m_Path);
        if (it.IsValid())
        {
          for (const xiiUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseThumbnailDeps.Find(pAssetInfo->m_Path);
        if (it2.IsValid())
        {
          for (const xiiUuid& guid : it2.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        break;
      }

      case xiiAssetInfo::TransformState::Unknown:
      {
        InvalidateAssetTransformState(assetGuid);
        break;
      }

      case xiiAssetInfo::TransformState::UpToDate:
      {
        if (bStateChanged)
        {
          xiiString sThumbPath = pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAssetInfo->m_Path);
          xiiQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);

          for (auto& subAssetUuid : pAssetInfo->m_SubAssets)
          {
            xiiSubAsset* pSubAsset;
            if (m_KnownSubAssets.TryGetValue(subAssetUuid, pSubAsset))
            {
              sThumbPath = pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAssetInfo->m_Path, pSubAsset->m_Data.m_sName);
              xiiQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);
            }
          }
        }
        break;
      }

      default:
        break;
    }
  }
}

void xiiAssetCurator::UpdateAssetTransformLog(const xiiUuid& assetGuid, xiiDynamicArray<xiiLogEntry>& logEntries)
{
  xiiAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    pAssetInfo->m_LogEntries.Clear();
    pAssetInfo->m_LogEntries.Swap(logEntries);
  }
}


void xiiAssetCurator::SetAssetExistanceState(xiiAssetInfo& assetInfo, xiiAssetExistanceState::Enum state)
{
  XII_ASSERT_DEBUG(m_CuratorMutex.IsLocked(), "");

  // Only the main thread tick function is allowed to change from FileAdded / FileRenamed to FileModified to inform views.
  // A modified 'added' file is still added until the added state was addressed.
  auto IsModifiedAfterAddOrRename = [](xiiAssetExistanceState::Enum oldState, xiiAssetExistanceState::Enum newState) -> bool {
    return oldState == xiiAssetExistanceState::FileAdded && newState == xiiAssetExistanceState::FileModified ||
      oldState == xiiAssetExistanceState::FileMoved && newState == xiiAssetExistanceState::FileModified;
  };

  if (!IsModifiedAfterAddOrRename(assetInfo.m_ExistanceState, state))
    assetInfo.m_ExistanceState = state;

  for (xiiUuid subGuid : assetInfo.m_SubAssets)
  {
    auto& existanceState = GetSubAssetInternal(subGuid)->m_ExistanceState;
    if (!IsModifiedAfterAddOrRename(existanceState, state))
    {
      existanceState = state;
      m_SubAssetChanged.Insert(subGuid);
    }
  }

  auto& existanceState = GetSubAssetInternal(assetInfo.m_Info->m_DocumentID)->m_ExistanceState;
  if (!IsModifiedAfterAddOrRename(existanceState, state))
  {
    existanceState = state;
    m_SubAssetChanged.Insert(assetInfo.m_Info->m_DocumentID);
  }
}


////////////////////////////////////////////////////////////////////////
// xiiUpdateTask
////////////////////////////////////////////////////////////////////////

xiiUpdateTask::xiiUpdateTask(xiiOnTaskFinishedCallback onTaskFinished)
{
  ConfigureTask("xiiUpdateTask", xiiTaskNesting::Maybe, onTaskFinished);
}

xiiUpdateTask::~xiiUpdateTask() = default;

void xiiUpdateTask::Execute()
{
  xiiUuid assetGuid;
  {
    XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
    if (!xiiAssetCurator::GetSingleton()->GetNextAssetToUpdate(assetGuid, m_sAssetPath))
      return;
  }

  const xiiDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  if (xiiDocumentManager::FindDocumentTypeFromPath(m_sAssetPath, false, pTypeDescriptor).Failed())
    return;

  xiiUInt64 uiAssetHash = 0;
  xiiUInt64 uiThumbHash = 0;

  // Do not log update errors done on the background thread. Only if done explicitly on the main thread or the GUI will not be responsive
  // if the user deleted some base asset and everything starts complaining about it.
  xiiLogEntryDelegate logger([&](xiiLogEntry& ref_entry) -> void {}, xiiLogMsgType::All);
  xiiLogSystemScope   logScope(&logger);

  xiiAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, xiiAssetCurator::GetSingleton()->GetActiveAssetProfile(), static_cast<const xiiAssetDocumentTypeDescriptor*>(pTypeDescriptor), uiAssetHash, uiThumbHash);
}
