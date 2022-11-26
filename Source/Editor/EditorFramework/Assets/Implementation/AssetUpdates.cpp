#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

////////////////////////////////////////////////////////////////////////
// xiiAssetCurator Asset Hashing and Status Updates
////////////////////////////////////////////////////////////////////////

xiiAssetInfo::TransformState xiiAssetCurator::HashAsset(xiiUInt64 uiSettingsHash, const xiiHybridArray<xiiString, 16>& assetTransformDependencies, const xiiHybridArray<xiiString, 16>& runtimeDependencies, xiiSet<xiiString>& missingDependencies, xiiSet<xiiString>& missingReferences, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("HashAsset");
  xiiStringBuilder             tmp;
  xiiAssetInfo::TransformState state = xiiAssetInfo::Unknown;
  {
    // hash of the main asset file
    out_AssetHash = uiSettingsHash;
    out_ThumbHash = uiSettingsHash;

    // Iterate dependencies
    for (const auto& dep : assetTransformDependencies)
    {
      xiiString sPath = dep;
      if (!AddAssetHash(sPath, false, out_AssetHash, out_ThumbHash, bForce))
      {
        missingDependencies.Insert(sPath);
      }
    }

    for (const auto& dep : runtimeDependencies)
    {
      xiiString sPath = dep;
      if (!AddAssetHash(sPath, true, out_AssetHash, out_ThumbHash, bForce))
      {
        missingReferences.Insert(sPath);
      }
    }
  }

  if (!missingReferences.IsEmpty())
  {
    out_ThumbHash = 0;
    state         = xiiAssetInfo::MissingReference;
  }
  if (!missingDependencies.IsEmpty())
  {
    out_AssetHash = 0;
    out_ThumbHash = 0;
    state         = xiiAssetInfo::MissingDependency;
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
    if (state == xiiAssetInfo::Unknown || state == xiiAssetInfo::MissingDependency || state == xiiAssetInfo::MissingReference)
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
  xiiFileStats statDep;
  if (xiiOSFile::GetFileStats(sPath, statDep).Failed())
  {
    xiiLog::Error("Failed to retrieve file stats '{0}'", sPath);
    return false;
  }

  xiiFileStatus fileStatus;
  xiiTimestamp  previousModificationTime;
  {
    XII_LOCK(m_CuratorMutex);
    fileStatus               = m_ReferencedFiles[sPath];
    previousModificationTime = fileStatus.m_Timestamp;
  }

  // if the file has been modified, make sure to get updated data
  if (!fileStatus.m_Timestamp.Compare(statDep.m_LastModificationTime, xiiTimestamp::CompareMode::Identical))
  {
    CURATOR_PROFILE(sPath);
    xiiFileReader file;
    if (file.Open(sPath).Failed())
    {
      xiiLog::Error("Failed to open file '{0}'", sPath);
      return false;
    }
    fileStatus.m_Timestamp = statDep.m_LastModificationTime;
    fileStatus.m_uiHash    = xiiAssetCurator::HashFile(file, nullptr);
    fileStatus.m_Status    = xiiFileStatus::Status::Valid;
  }

  {
    XII_LOCK(m_CuratorMutex);
    xiiFileStatus& refFile = m_ReferencedFiles[sPath];
    // Only update the status if the file status has not been changed between the locks or we might write stale data to it.
    if (refFile.m_Timestamp.Compare(previousModificationTime, xiiTimestamp::CompareMode::Identical))
    {
      refFile = fileStatus;
    }
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

xiiResult xiiAssetCurator::EnsureAssetInfoUpdated(const xiiUuid& assetGuid)
{
  XII_LOCK(m_CuratorMutex);
  xiiAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return XII_FAILURE;

  // It is not safe here to pass pInfo->m_sAbsolutePath into EnsureAssetInfoUpdated
  // as the function is meant to change the very instance we are passing in.
  xiiStringBuilder sAbsPath = pInfo->m_sAbsolutePath;
  return EnsureAssetInfoUpdated(sAbsPath);
}

static xiiResult PatchAssetGuid(const char* szAbsFilePath, xiiUuid oldGuid, xiiUuid newGuid)
{
  const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (xiiDocumentManager::FindDocumentTypeFromPath(szAbsFilePath, true, pTypeDesc).Failed())
    return XII_FAILURE;

  if (xiiDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(szAbsFilePath))
  {
    pTypeDesc->m_pManager->CloseDocument(pDocument);
  }

  xiiUInt32 uiTries = 0;
  while (pTypeDesc->m_pManager->CloneDocument(szAbsFilePath, szAbsFilePath, newGuid).Failed())
  {
    if (uiTries >= 5)
      return XII_FAILURE;

    xiiThreadUtils::Sleep(xiiTime::Milliseconds(50 * (uiTries + 1)));
    uiTries++;
  }

  return XII_SUCCESS;
}

xiiResult xiiAssetCurator::EnsureAssetInfoUpdated(const char* szAbsFilePath)
{
  CURATOR_PROFILE(szAbsFilePath);
  xiiFileStats fs;
  {
    CURATOR_PROFILE("GetFileStats");
    if (xiiOSFile::GetFileStats(szAbsFilePath, fs).Failed())
      return XII_FAILURE;
  }
  {
    // If the file stat matches our stored timestamp, we are still up to date.
    XII_LOCK(m_CuratorMutex);
    if (m_ReferencedFiles[szAbsFilePath].m_Timestamp.Compare(fs.m_LastModificationTime, xiiTimestamp::CompareMode::Identical))
      return XII_SUCCESS;
  }

  // Read document info outside the lock
  xiiFileStatus              fileStatus;
  xiiUniquePtr<xiiAssetInfo> pNewAssetInfo;
  XII_SUCCEED_OR_RETURN(ReadAssetDocumentInfo(szAbsFilePath, fileStatus, pNewAssetInfo));
  XII_ASSERT_DEV(pNewAssetInfo != nullptr && pNewAssetInfo->m_Info != nullptr, "Info should be valid on success.");

  XII_LOCK(m_CuratorMutex);
  xiiFileStatus& RefFile = m_ReferencedFiles[szAbsFilePath];
  xiiUuid        oldGuid = RefFile.m_AssetGuid;
  // if it already has a valid GUID, an xiiAssetInfo object must exist
  bool bNew = !RefFile.m_AssetGuid.IsValid(); // Under this current location the asset is not known.
  XII_VERIFY(bNew == !m_KnownAssets.Contains(RefFile.m_AssetGuid), "guid set in file-status but no asset is actually known under that guid");

  RefFile                     = fileStatus;
  xiiAssetInfo* pOldAssetInfo = nullptr;
  if (bNew)
  {
    // now the GUID must be valid
    XII_ASSERT_DEV(pNewAssetInfo->m_Info->m_DocumentID.IsValid(), "Asset header read for '{0}', but its GUID is invalid! Corrupted document?", szAbsFilePath);
    XII_ASSERT_DEV(RefFile.m_AssetGuid == pNewAssetInfo->m_Info->m_DocumentID, "UpdateAssetInfo broke the GUID!");

    // Was the asset already known? Decide whether it was moved (ok) or duplicated (bad)
    m_KnownAssets.TryGetValue(pNewAssetInfo->m_Info->m_DocumentID, pOldAssetInfo);
    if (pOldAssetInfo != nullptr)
    {
      if (pNewAssetInfo->m_sAbsolutePath == pOldAssetInfo->m_sAbsolutePath)
      {
        // As it is a new asset, this should actually never be the case.
        UntrackDependencies(pOldAssetInfo);
        pOldAssetInfo->Update(pNewAssetInfo);
        TrackDependencies(pOldAssetInfo);
        UpdateAssetTransformState(RefFile.m_AssetGuid, xiiAssetInfo::TransformState::Unknown);
        SetAssetExistanceState(*pOldAssetInfo, xiiAssetExistanceState::FileModified);
        UpdateSubAssets(*pOldAssetInfo);
        RefFile.m_AssetGuid = pOldAssetInfo->m_Info->m_DocumentID;
        return XII_SUCCESS;
      }
      else
      {
        xiiFileStats fsOldLocation;
        if (xiiOSFile::GetFileStats(pOldAssetInfo->m_sAbsolutePath, fsOldLocation).Failed())
        {
          // Asset moved, remove old file and asset info.
          m_ReferencedFiles.Remove(pOldAssetInfo->m_sAbsolutePath);
          UntrackDependencies(pOldAssetInfo);
          pOldAssetInfo->Update(pNewAssetInfo);
          TrackDependencies(pOldAssetInfo);
          UpdateAssetTransformState(RefFile.m_AssetGuid, xiiAssetInfo::TransformState::Unknown);
          SetAssetExistanceState(*pOldAssetInfo,
                                 xiiAssetExistanceState::FileModified); // asset was only moved, prevent added event (could have been modified though)
          UpdateSubAssets(*pOldAssetInfo);
          RefFile.m_AssetGuid = pOldAssetInfo->m_Info->m_DocumentID;
          return XII_SUCCESS;
        }
        else
        {
          // Unfortunately we only know about duplicates in the order in which the filesystem tells us about files
          // That means we currently always adjust the GUID of the second, third, etc. file that we look at
          // even if we might know that changing another file makes more sense
          // This works well for when the editor is running and someone copies a file.

          xiiLog::Error("Two assets have identical GUIDs: '{0}' and '{1}'", pNewAssetInfo->m_sAbsolutePath, pOldAssetInfo->m_sAbsolutePath);

          const xiiUuid mod     = xiiUuid::StableUuidForString(szAbsFilePath);
          xiiUuid       newGuid = pNewAssetInfo->m_Info->m_DocumentID;
          newGuid.CombineWithSeed(mod);

          if (PatchAssetGuid(szAbsFilePath, pNewAssetInfo->m_Info->m_DocumentID, newGuid).Failed())
          {
            xiiLog::Error("Failed to adjust GUID of asset: '{0}'", szAbsFilePath);
            m_ReferencedFiles.Remove(szAbsFilePath);
            return XII_FAILURE;
          }

          xiiLog::Warning("Adjusted GUID of asset to make it unique: '{0}'", szAbsFilePath);

          // now let's try that again
          m_ReferencedFiles.Remove(szAbsFilePath);
          return EnsureAssetInfoUpdated(szAbsFilePath);
        }
      }
    }

    // and we can store the new xiiAssetInfo data under that GUID
    pOldAssetInfo                      = pNewAssetInfo.Release();
    m_KnownAssets[RefFile.m_AssetGuid] = pOldAssetInfo;
    TrackDependencies(pOldAssetInfo);
    UpdateAssetTransformState(pOldAssetInfo->m_Info->m_DocumentID, xiiAssetInfo::TransformState::Unknown);
    UpdateSubAssets(*pOldAssetInfo);
  }
  else
  {
    // Guid changed, different asset found, mark old as deleted and add new one.
    if (oldGuid != RefFile.m_AssetGuid)
    {
      SetAssetExistanceState(*m_KnownAssets[oldGuid], xiiAssetExistanceState::FileRemoved);
      RemoveAssetTransformState(oldGuid);

      if (RefFile.m_AssetGuid.IsValid())
      {
        pOldAssetInfo                      = pNewAssetInfo.Release();
        m_KnownAssets[RefFile.m_AssetGuid] = pOldAssetInfo;
        TrackDependencies(pOldAssetInfo);
        // Don't call SetAssetExistanceState on newly created assets as their data structure is initialized in UpdateSubAssets for the first time.
        UpdateSubAssets(*pOldAssetInfo);
      }
    }
    else
    {
      // Update asset info
      pOldAssetInfo = m_KnownAssets[RefFile.m_AssetGuid];
      UntrackDependencies(pOldAssetInfo);
      pOldAssetInfo->Update(pNewAssetInfo);
      TrackDependencies(pOldAssetInfo);
      SetAssetExistanceState(*pOldAssetInfo, xiiAssetExistanceState::FileModified);
      UpdateSubAssets(*pOldAssetInfo);
    }
  }

  InvalidateAssetTransformState(RefFile.m_AssetGuid);
  return XII_SUCCESS;
}

void xiiAssetCurator::TrackDependencies(xiiAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_AssetTransformDependencies, m_InverseDependency, m_UnresolvedDependencies, true);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_RuntimeDependencies, m_InverseReferences, m_UnresolvedReferences, true);

  const xiiString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, "");
  auto            it          = m_InverseReferences.FindOrAdd(sTargetFile);
  it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const xiiString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it                           = m_InverseReferences.FindOrAdd(sTargetFile2);
    it.Value().PushBack(pAssetInfo->m_Info->m_DocumentID);
  }

  UpdateUnresolvedTrackedFiles(m_InverseDependency, m_UnresolvedDependencies);
  UpdateUnresolvedTrackedFiles(m_InverseReferences, m_UnresolvedReferences);
}

void xiiAssetCurator::UntrackDependencies(xiiAssetInfo* pAssetInfo)
{
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_AssetTransformDependencies, m_InverseDependency, m_UnresolvedDependencies, false);
  UpdateTrackedFiles(pAssetInfo->m_Info->m_DocumentID, pAssetInfo->m_Info->m_RuntimeDependencies, m_InverseReferences, m_UnresolvedReferences, false);

  const xiiString sTargetFile = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, "");
  auto            it          = m_InverseReferences.FindOrAdd(sTargetFile);
  it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  for (auto outputIt = pAssetInfo->m_Info->m_Outputs.GetIterator(); outputIt.IsValid(); ++outputIt)
  {
    const xiiString sTargetFile2 = pAssetInfo->GetManager()->GetAbsoluteOutputFileName(pAssetInfo->m_pDocumentTypeDescriptor, pAssetInfo->m_sAbsolutePath, outputIt.Key());
    it                           = m_InverseReferences.FindOrAdd(sTargetFile2);
    it.Value().RemoveAndCopy(pAssetInfo->m_Info->m_DocumentID);
  }
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

      sPath = pInfo->m_sAbsolutePath;
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
      xiiString sPath     = pInfo->m_sAbsolutePath;
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

xiiResult xiiAssetCurator::ReadAssetDocumentInfo(const char* szAbsFilePath, xiiFileStatus& stat, xiiUniquePtr<xiiAssetInfo>& out_assetInfo)
{
  CURATOR_PROFILE(szAbsFilePath);

  xiiFileStats fs;
  if (xiiOSFile::GetFileStats(szAbsFilePath, fs).Failed())
    return XII_FAILURE;
  stat.m_Timestamp = fs.m_LastModificationTime;
  stat.m_Status    = xiiFileStatus::Status::Valid;

  // try to read the asset file
  xiiFileReader file;
  if (file.Open(szAbsFilePath) == XII_FAILURE)
  {
    stat.m_Timestamp.Invalidate();
    stat.m_uiHash = 0;
    stat.m_Status = xiiFileStatus::Status::FileLocked;

    xiiLog::Error("Failed to open asset file '{0}'", szAbsFilePath);
    return XII_FAILURE;
  }

  out_assetInfo = XII_DEFAULT_NEW(xiiAssetInfo);
  xiiUniquePtr<xiiAssetDocumentInfo> docInfo;
  auto                               itFile = m_CachedFiles.Find(szAbsFilePath);
  {
    XII_LOCK(m_CachedAssetsMutex);
    auto itAsset = m_CachedAssets.Find(szAbsFilePath);
    if (itAsset.IsValid())
    {
      docInfo = std::move(itAsset.Value());
      m_CachedAssets.Remove(itAsset);
    }
  }

  // update the paths
  {
    xiiStringBuilder sDataDir = GetSingleton()->FindDataDirectoryForAsset(szAbsFilePath);
    sDataDir.PathParentDirectory();

    xiiStringBuilder sRelPath = szAbsFilePath;
    sRelPath.MakeRelativeTo(sDataDir).IgnoreResult();

    out_assetInfo->m_sDataDirParentRelativePath = sRelPath;
    out_assetInfo->m_sDataDirRelativePath       = xiiStringView(out_assetInfo->m_sDataDirParentRelativePath.FindSubString("/") + 1);
    out_assetInfo->m_sAbsolutePath              = szAbsFilePath;
  }

  // figure out which manager should handle this asset type
  {
    const xiiDocumentTypeDescriptor* pTypeDesc = nullptr;
    if (out_assetInfo->m_pDocumentTypeDescriptor == nullptr)
    {
      if (xiiDocumentManager::FindDocumentTypeFromPath(szAbsFilePath, false, pTypeDesc).Failed())
      {
        XII_REPORT_FAILURE("Invalid asset setup");
      }

      out_assetInfo->m_pDocumentTypeDescriptor = static_cast<const xiiAssetDocumentTypeDescriptor*>(pTypeDesc);
    }
  }

  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamReader         MemReader(&storage);
  MemReader.SetDebugSourceInformation(out_assetInfo->m_sAbsolutePath);

  xiiMemoryStreamWriter MemWriter(&storage);

  if (docInfo && itFile.IsValid() && itFile.Value().m_Timestamp.Compare(stat.m_Timestamp, xiiTimestamp::CompareMode::Identical))
  {
    stat.m_uiHash = itFile.Value().m_uiHash;
  }
  else
  {
    // compute the hash for the asset file
    stat.m_uiHash = xiiAssetCurator::HashFile(file, &MemWriter);
  }
  file.Close();

  // and finally actually read the asset file (header only) and store the information in the xiiAssetDocumentInfo member
  if (docInfo && itFile.IsValid() && itFile.Value().m_Timestamp.Compare(stat.m_Timestamp, xiiTimestamp::CompareMode::Identical))
  {
    out_assetInfo->m_Info = std::move(docInfo);
    stat.m_AssetGuid      = out_assetInfo->m_Info->m_DocumentID;
  }
  else
  {
    xiiStatus ret = out_assetInfo->GetManager()->ReadAssetDocumentInfo(out_assetInfo->m_Info, MemReader);
    if (ret.Failed())
    {
      xiiLog::Error("Failed to read asset document info for asset file '{0}'", szAbsFilePath);
      return XII_FAILURE;
    }
    XII_ASSERT_DEV(out_assetInfo->m_Info != nullptr, "Info should be valid on suceess.");

    // here we get the GUID out of the document
    // this links the 'file' to the 'asset'
    stat.m_AssetGuid = out_assetInfo->m_Info->m_DocumentID;
  }

  return XII_SUCCESS;
}

void xiiAssetCurator::UpdateSubAssets(xiiAssetInfo& assetInfo)
{
  CURATOR_PROFILE("UpdateSubAssets");
  if (assetInfo.m_ExistanceState == xiiAssetExistanceState::FileRemoved)
  {
    return;
  }

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

xiiUInt64 xiiAssetCurator::HashFile(xiiStreamReader& InputStream, xiiStreamWriter* pPassThroughStream)
{
  xiiHashStreamWriter64 hsw;

  CURATOR_PROFILE("HashFile");
  xiiUInt8 cachedBytes[1024 * 10];

  while (true)
  {
    const xiiUInt64 uiRead = InputStream.ReadBytes(cachedBytes, XII_ARRAY_SIZE(cachedBytes));

    if (uiRead == 0)
      break;

    hsw.WriteBytes(cachedBytes, uiRead).IgnoreResult();

    if (pPassThroughStream != nullptr)
      pPassThroughStream->WriteBytes(cachedBytes, uiRead).IgnoreResult();
  }

  return hsw.GetHashValue();
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

  xiiAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
  {
    // We do not set pAssetInfo->m_TransformState because that is user facing and
    // as after updating the state it might just be the same as before we instead add
    // it to the queue here to prevent flickering in the GUI.
    m_TransformStateStale.Insert(assetGuid);
    // Increasing m_LastStateUpdate will ensure that asset hash/state computations
    // that are in flight will not be written back to the asset.
    pAssetInfo->m_LastStateUpdate++;
    pAssetInfo->m_AssetHash = 0;
    pAssetInfo->m_ThumbHash = 0;
    auto it                 = m_InverseDependency.Find(pAssetInfo->m_sAbsolutePath);
    if (it.IsValid())
    {
      for (const xiiUuid& guid : it.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }

    auto it2 = m_InverseReferences.Find(pAssetInfo->m_sAbsolutePath);
    if (it2.IsValid())
    {
      for (const xiiUuid& guid : it2.Value())
      {
        InvalidateAssetTransformState(guid);
      }
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
        auto it = m_InverseDependency.Find(pAssetInfo->m_sAbsolutePath);
        if (it.IsValid())
        {
          for (const xiiUuid& guid : it.Value())
          {
            InvalidateAssetTransformState(guid);
          }
        }

        auto it2 = m_InverseReferences.Find(pAssetInfo->m_sAbsolutePath);
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
          xiiString sThumbPath = static_cast<xiiAssetDocumentManager*>(pAssetInfo->GetManager())->GenerateResourceThumbnailPath(pAssetInfo->m_sAbsolutePath);
          xiiQtImageCache::GetSingleton()->InvalidateCache(sThumbPath);
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

  // Only the main thread tick function is allowed to change from FileAdded to FileModified to inform views.
  // A modified 'added' file is still added until the added state was addressed.
  if (assetInfo.m_ExistanceState != xiiAssetExistanceState::FileAdded || state != xiiAssetExistanceState::FileModified)
    assetInfo.m_ExistanceState = state;

  for (xiiUuid subGuid : assetInfo.m_SubAssets)
  {
    auto& existanceState = GetSubAssetInternal(subGuid)->m_ExistanceState;
    if (existanceState != xiiAssetExistanceState::FileAdded || state != xiiAssetExistanceState::FileModified)
    {
      existanceState = state;
      m_SubAssetChanged.Insert(subGuid);
    }
  }

  auto& existanceState = GetSubAssetInternal(assetInfo.m_Info->m_DocumentID)->m_ExistanceState;
  if (existanceState != xiiAssetExistanceState::FileAdded || state != xiiAssetExistanceState::FileModified)
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
  ConfigureTask("xiiUpdateTask", xiiTaskNesting::Never, onTaskFinished);
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
  xiiLogEntryDelegate logger([&](xiiLogEntry& entry) -> void {}, xiiLogMsgType::All);
  xiiLogSystemScope   logScope(&logger);

  xiiAssetCurator::GetSingleton()->IsAssetUpToDate(assetGuid, xiiAssetCurator::GetSingleton()->GetActiveAssetProfile(), static_cast<const xiiAssetDocumentTypeDescriptor*>(pTypeDescriptor), uiAssetHash, uiThumbHash);
}
