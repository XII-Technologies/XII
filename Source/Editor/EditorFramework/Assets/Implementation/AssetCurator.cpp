#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/Assets/AssetWatcher.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#define XII_CURATOR_CACHE_VERSION      2
#define XII_CURATOR_CACHE_FILE_VERSION 6

XII_IMPLEMENT_SINGLETON(xiiAssetCurator);

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, AssetCurator)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiAssetCurator);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiAssetCurator* pDummy = xiiAssetCurator::GetSingleton();
    XII_DEFAULT_DELETE(pDummy);
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiFileStatus, xiiNoBase, 3, xiiRTTIDefaultAllocator<xiiFileStatus>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Timestamp", m_Timestamp),
    XII_MEMBER_PROPERTY("Hash", m_uiHash),
    XII_MEMBER_PROPERTY("AssetGuid", m_AssetGuid),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

inline xiiStreamWriter& operator<<(xiiStreamWriter& Stream, const xiiFileStatus& uiValue)
{
  Stream.WriteBytes(&uiValue, sizeof(xiiFileStatus)).IgnoreResult();
  return Stream;
}

inline xiiStreamReader& operator>>(xiiStreamReader& Stream, xiiFileStatus& uiValue)
{
  Stream.ReadBytes(&uiValue, sizeof(xiiFileStatus));
  return Stream;
}

void xiiAssetInfo::Update(xiiUniquePtr<xiiAssetInfo>& rhs)
{
  m_ExistanceState             = rhs->m_ExistanceState;
  m_TransformState             = rhs->m_TransformState;
  m_pDocumentTypeDescriptor    = rhs->m_pDocumentTypeDescriptor;
  m_sAbsolutePath              = std::move(rhs->m_sAbsolutePath);
  m_sDataDirParentRelativePath = std::move(rhs->m_sDataDirParentRelativePath);
  m_sDataDirRelativePath       = xiiStringView(m_sDataDirParentRelativePath.FindSubString("/") + 1); // skip the initial folder
  m_Info                       = std::move(rhs->m_Info);
  m_AssetHash                  = rhs->m_AssetHash;
  m_ThumbHash                  = rhs->m_ThumbHash;
  m_MissingDependencies        = rhs->m_MissingDependencies;
  m_MissingReferences          = rhs->m_MissingReferences;
  // Don't copy m_SubAssets, we want to update it independently.
  rhs = nullptr;
}

xiiStringView xiiSubAsset::GetName() const
{
  if (m_bMainAsset)
    return xiiPathUtils::GetFileName(m_pAssetInfo->m_sDataDirParentRelativePath);
  else
    return m_Data.m_sName;
}


void xiiSubAsset::GetSubAssetIdentifier(xiiStringBuilder& out_sPath) const
{
  out_sPath = m_pAssetInfo->m_sDataDirParentRelativePath;

  if (!m_bMainAsset)
  {
    out_sPath.Append("|", m_Data.m_sName);
  }
}

////////////////////////////////////////////////////////////////////////
// xiiAssetCurator Setup
////////////////////////////////////////////////////////////////////////

xiiAssetCurator::xiiAssetCurator() :
  m_SingletonRegistrar(this)
{
}

xiiAssetCurator::~xiiAssetCurator()
{
  XII_ASSERT_DEBUG(m_KnownAssets.IsEmpty(), "Need to call Deinitialize before curator is deleted.");
}

void xiiAssetCurator::StartInitialize(const xiiApplicationFileSystemConfig& cfg)
{
  XII_PROFILE_SCOPE("StartInitialize");

  {
    XII_LOG_BLOCK("SetupAssetProfiles");

    SetupDefaultAssetProfiles();
    if (LoadAssetProfiles().Failed())
    {
      xiiLog::Warning("Asset profiles file does not exist or contains invalid data. Setting up default profiles.");
      SaveAssetProfiles().IgnoreResult();
      SaveRuntimeProfiles();
    }
  }

  ComputeAllDocumentManagerAssetProfileHashes();
  BuildFileExtensionSet(m_ValidAssetExtensions);

  m_bRunUpdateTask   = true;
  m_FileSystemConfig = cfg;

  m_pWatcher = XII_DEFAULT_NEW(xiiAssetWatcher, m_FileSystemConfig);

  xiiSharedPtr<xiiDelegateTask<void>> pInitTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "AssetCuratorUpdateCache", [this]() {
    XII_LOCK(m_CuratorMutex);
    LoadCaches();

    m_CuratorMutex.Unlock();
    CheckFileSystem();
    m_CuratorMutex.Lock();

    // As we fired a AssetListReset in CheckFileSystem, set everything new to FileUnchanged or
    // we would fire an added call for every asset.
    for (auto it = m_KnownSubAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value().m_ExistanceState == xiiAssetExistanceState::FileAdded)
      {
        it.Value().m_ExistanceState = xiiAssetExistanceState::FileUnchanged;
      }
    }
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_ExistanceState == xiiAssetExistanceState::FileAdded)
      {
        it.Value()->m_ExistanceState = xiiAssetExistanceState::FileUnchanged;
      }
    }
    SaveCaches(); });
  pInitTask->ConfigureTask("Initialize Curator", xiiTaskNesting::Never);
  m_InitializeCuratorTaskID = xiiTaskSystem::StartSingleTask(pInitTask, xiiTaskPriority::FileAccessHighPriority);

  {
    xiiAssetCuratorEvent e;
    e.m_Type = xiiAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }
}

void xiiAssetCurator::WaitForInitialize()
{
  XII_PROFILE_SCOPE("WaitForInitialize");
  xiiTaskSystem::WaitForGroup(m_InitializeCuratorTaskID);
  m_InitializeCuratorTaskID.Invalidate();

  XII_LOCK(m_CuratorMutex);
  ProcessAllCoreAssets();
  // Broadcast reset.
  {
    xiiAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type  = xiiAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }
}

void xiiAssetCurator::Deinitialize()
{
  XII_PROFILE_SCOPE("Deinitialize");

  SaveAssetProfiles().IgnoreResult();

  ShutdownUpdateTask();
  xiiAssetProcessor::GetSingleton()->StopProcessTask(true);
  m_pWatcher = nullptr;

  SaveCaches();

  {
    m_ReferencedFiles.Clear();

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      XII_DEFAULT_DELETE(it.Value());
    }
    m_KnownSubAssets.Clear();
    m_KnownAssets.Clear();
    m_TransformStateStale.Clear();

    for (int i = 0; i < xiiAssetInfo::TransformState::COUNT; i++)
    {
      m_TransformState[i].Clear();
    }
  }

  // Broadcast reset.
  {
    xiiAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type  = xiiAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  ClearAssetProfiles();
}

void xiiAssetCurator::MainThreadTick(bool bTopLevel)
{
  CURATOR_PROFILE("MainThreadTick");

  static std::atomic<bool> bReentry = false;
  if (bReentry)
    return;

  if (xiiQtEditorApp::GetSingleton()->IsProgressBarProcessingEvents())
    return;

  bReentry = true;

  if (m_pWatcher)
    m_pWatcher->MainThreadTick();

  XII_LOCK(m_CuratorMutex);
  xiiHybridArray<xiiAssetInfo*, 32> deletedAssets;
  for (const xiiUuid& guid : m_SubAssetChanged)
  {
    xiiSubAsset*         pInfo = GetSubAssetInternal(guid);
    xiiAssetCuratorEvent e;
    e.m_AssetGuid = guid;
    e.m_pInfo     = pInfo;
    e.m_Type      = xiiAssetCuratorEvent::Type::AssetUpdated;

    if (pInfo != nullptr)
    {
      if (pInfo->m_ExistanceState == xiiAssetExistanceState::FileAdded)
      {
        pInfo->m_ExistanceState = xiiAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = xiiAssetExistanceState::FileUnchanged;
        e.m_Type = xiiAssetCuratorEvent::Type::AssetAdded;
        m_Events.Broadcast(e);
      }
      else if (pInfo->m_ExistanceState == xiiAssetExistanceState::FileRemoved)
      {
        e.m_Type = xiiAssetCuratorEvent::Type::AssetRemoved;
        m_Events.Broadcast(e);

        if (pInfo->m_bMainAsset)
        {
          deletedAssets.PushBack(pInfo->m_pAssetInfo);
        }
        m_KnownAssets.Remove(guid);
        m_KnownSubAssets.Remove(guid);
      }
      else // Either xiiAssetInfo::ExistanceState::FileModified or tranform changed
      {
        pInfo->m_ExistanceState = xiiAssetExistanceState::FileUnchanged;
        if (pInfo->m_bMainAsset)
          pInfo->m_pAssetInfo->m_ExistanceState = xiiAssetExistanceState::FileUnchanged;
        e.m_Type = xiiAssetCuratorEvent::Type::AssetUpdated;
        m_Events.Broadcast(e);
      }
    }
  }
  m_SubAssetChanged.Clear();

  // Delete file asset info after all the sub-assets have been handled (so no ref exist to it anymore).
  for (xiiAssetInfo* pInfo : deletedAssets)
  {
    XII_DEFAULT_DELETE(pInfo);
  }

  RunNextUpdateTask();

  if (bTopLevel && !m_TransformState[xiiAssetInfo::TransformState::NeedsImport].IsEmpty())
  {
    const xiiUuid assetToImport = *m_TransformState[xiiAssetInfo::TransformState::NeedsImport].GetIterator();

    xiiAssetInfo* pInfo = GetAssetInfo(assetToImport);

    ProcessAsset(pInfo, nullptr, xiiTransformFlags::TriggeredManually);
    UpdateAssetTransformState(assetToImport, xiiAssetInfo::TransformState::Unknown);
  }

  // TODO: Probably needs to be done in headless as well to make proper thumbnails
  if (!xiiQtEditorApp::GetSingleton()->IsInHeadlessMode())
  {
    if (bTopLevel && m_bNeedToReloadResources && xiiTime::Now() > m_NextReloadResources)
    {
      m_bNeedToReloadResources = false;
      WriteAssetTables().IgnoreResult();
    }
  }

  bReentry = false;
}

xiiDateTime xiiAssetCurator::GetLastFullTransformDate() const
{
  xiiStringBuilder path = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  xiiFileStats stat;
  if (xiiOSFile::GetFileStats(path, stat).Failed())
    return {};

  return stat.m_LastModificationTime;
}

void xiiAssetCurator::StoreFullTransformDate()
{
  xiiStringBuilder path = xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  path.AppendPath("LastFullTransform.date");

  xiiOSFile file;
  if (file.Open(path, xiiFileOpenMode::Write).Succeeded())
  {
    xiiDateTime date;
    date.SetTimestamp(xiiTimestamp::CurrentTimestamp());

    path.Format("{}", date);
    file.Write(path.GetData(), path.GetElementCount()).AssertSuccess();
  }
}

////////////////////////////////////////////////////////////////////////
// xiiAssetCurator High Level Functions
////////////////////////////////////////////////////////////////////////

xiiStatus xiiAssetCurator::TransformAllAssets(xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile)
{
  XII_PROFILE_SCOPE("TransformAllAssets");

  xiiDynamicArray<xiiUuid> assets;
  {
    XII_LOCK(m_CuratorMutex);
    assets.Reserve(m_KnownAssets.GetCount());
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      assets.PushBack(it.Key());
    }
  }
  xiiUInt32 uiNumStepsLeft = assets.GetCount();

  xiiUInt32        uiNumFailedSteps = 0;
  xiiProgressRange range("Transforming Assets", 1 + uiNumStepsLeft, true);
  for (const xiiUuid& assetGuid : assets)
  {
    if (range.WasCanceled())
      break;

    XII_LOCK(m_CuratorMutex);

    xiiAssetInfo* pAssetInfo = nullptr;
    if (!m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
      continue;

    if (uiNumStepsLeft > 0)
    {
      // it can happen that the number of known assets changes while we are processing them
      // in this case the progress bar may assert that the number of steps completed is larger than
      // what was specified before
      // since this is a valid case, we just stop updating the progress bar, in case more assets are detected

      range.BeginNextStep(xiiPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirParentRelativePath).GetStartPointer());
      --uiNumStepsLeft;
    }

    xiiTransformStatus res = ProcessAsset(pAssetInfo, pAssetProfile, transformFlags);
    if (res.Failed())
    {
      uiNumFailedSteps++;
      xiiLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_sDataDirParentRelativePath);
    }
  }

  range.BeginNextStep("Writing Lookup Tables");

  WriteAssetTables(pAssetProfile).IgnoreResult();

  StoreFullTransformDate();

  if (uiNumFailedSteps > 0)
    return xiiStatus(xiiFmt("Transform all assets failed on {0} assets.", uiNumFailedSteps));

  return xiiStatus(XII_SUCCESS);
}

void xiiAssetCurator::ResaveAllAssets()
{
  xiiProgressRange range("Re-saving all Assets", 1 + m_KnownAssets.GetCount(), true);

  XII_LOCK(m_CuratorMutex);

  xiiDynamicArray<xiiUuid> sortedAssets;
  sortedAssets.Reserve(m_KnownAssets.GetCount());

  xiiMap<xiiUuid, xiiSet<xiiUuid>> dependencies;

  xiiSet<xiiUuid> accu;

  for (auto itAsset = m_KnownAssets.GetIterator(); itAsset.IsValid(); ++itAsset)
  {
    auto it2 = dependencies.Insert(itAsset.Key(), xiiSet<xiiUuid>());
    for (const xiiString& dep : itAsset.Value()->m_Info->m_AssetTransformDependencies)
    {
      if (xiiConversionUtils::IsStringUuid(dep))
      {
        it2.Value().Insert(xiiConversionUtils::ConvertStringToUuid(dep));
      }
    }
  }

  while (!dependencies.IsEmpty())
  {
    bool bDeadEnd = true;
    for (auto it = dependencies.GetIterator(); it.IsValid(); ++it)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(it.Value()))
      {
        sortedAssets.PushBack(it.Key());
        accu.Insert(it.Key());
        dependencies.Remove(it);
        bDeadEnd = false;
        break;
      }
    }

    if (bDeadEnd)
    {
      // Just take the next one in and hope for the best.
      auto it = dependencies.GetIterator();
      sortedAssets.PushBack(it.Key());
      accu.Insert(it.Key());
      dependencies.Remove(it);
    }
  }

  for (xiiUInt32 i = 0; i < sortedAssets.GetCount(); i++)
  {
    if (range.WasCanceled())
      break;

    xiiAssetInfo* pAssetInfo = GetAssetInfo(sortedAssets[i]);
    XII_ASSERT_DEBUG(pAssetInfo, "Should not happen as data was derived from known assets list.");
    range.BeginNextStep(xiiPathUtils::GetFileNameAndExtension(pAssetInfo->m_sDataDirParentRelativePath).GetStartPointer());

    auto res = ResaveAsset(pAssetInfo);
    if (res.m_Result.Failed())
    {
      xiiLog::Error("{0} ({1})", res.m_sMessage, pAssetInfo->m_sDataDirParentRelativePath);
    }
  }
}

xiiTransformStatus xiiAssetCurator::TransformAsset(const xiiUuid& assetGuid, xiiBitflags<xiiTransformFlags> transformFlags, const xiiPlatformProfile* pAssetProfile)
{
  xiiTransformStatus                    res;
  xiiStringBuilder                      sAbsPath;
  xiiStopwatch                          timer;
  const xiiAssetDocumentTypeDescriptor* pTypeDesc = nullptr;
  {
    XII_LOCK(m_CuratorMutex);

    xiiAssetInfo* pInfo = nullptr;
    if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
      return xiiTransformStatus("Transform failed, unknown asset.");

    sAbsPath = pInfo->m_sAbsolutePath;
    res      = ProcessAsset(pInfo, pAssetProfile, transformFlags);
  }
  if (pTypeDesc && transformFlags.IsAnySet(xiiTransformFlags::TriggeredManually))
  {
    // As this is triggered manually it is safe to save here as these are only run on the main thread.
    if (xiiDocument* pDoc = pTypeDesc->m_pManager->GetDocumentByPath(sAbsPath))
    {
      // some assets modify the document during transformation
      // make sure the state is saved, at least when the user actively executed the action
      pDoc->SaveDocument();
    }
  }
  xiiLog::Info("Transform asset time: {0}s", xiiArgF(timer.GetRunningTotal().GetSeconds(), 2));
  return res;
}

xiiTransformStatus xiiAssetCurator::CreateThumbnail(const xiiUuid& assetGuid)
{
  XII_LOCK(m_CuratorMutex);

  xiiAssetInfo* pInfo = nullptr;
  if (!m_KnownAssets.TryGetValue(assetGuid, pInfo))
    return xiiStatus("Create thumbnail failed, unknown asset.");

  return ProcessAsset(pInfo, nullptr, xiiTransformFlags::None);
}

xiiResult xiiAssetCurator::WriteAssetTables(const xiiPlatformProfile* pAssetProfile /* = nullptr*/)
{
  CURATOR_PROFILE("WriteAssetTables");
  XII_LOG_BLOCK("xiiAssetCurator::WriteAssetTables");

  // TODO: figure out a way to early out this function, if nothing can have changed

  xiiResult res = XII_SUCCESS;

  xiiStringBuilder sd;

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    XII_SUCCEED_OR_RETURN(xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sd));
    sd.Append("/");

    if (WriteAssetTable(sd, pAssetProfile).Failed())
      res = XII_FAILURE;
  }

  if (pAssetProfile == nullptr || pAssetProfile == GetActiveAssetProfile())
  {
    xiiSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadAssetLUT";
    msg.m_sPayload  = GetActiveAssetProfile()->GetConfigName();
    xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);

    msg.m_sWhatToDo = "ReloadResources";
    xiiEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  return res;
}


////////////////////////////////////////////////////////////////////////
// xiiAssetCurator Asset Access
////////////////////////////////////////////////////////////////////////

const xiiAssetCurator::xiiLockedSubAsset xiiAssetCurator::FindSubAsset(const char* szPathOrGuid, bool bExhaustiveSearch) const
{
  CURATOR_PROFILE("FindSubAsset");
  XII_LOCK(m_CuratorMutex);

  if (xiiConversionUtils::IsStringUuid(szPathOrGuid))
  {
    return GetSubAsset(xiiConversionUtils::ConvertStringToUuid(szPathOrGuid));
  }

  // Split into mainAsset|subAsset
  xiiStringBuilder mainAsset;
  xiiStringView    subAsset;
  const char*      szSeparator = xiiStringUtils::FindSubString(szPathOrGuid, "|");
  if (szSeparator != nullptr)
  {
    mainAsset.SetSubString_FromTo(szPathOrGuid, szSeparator);
    subAsset = xiiStringView(szSeparator + 1);
  }
  else
  {
    mainAsset = szPathOrGuid;
  }
  mainAsset.MakeCleanPath();

  // Find mainAsset
  xiiMap<xiiString, xiiFileStatus, xiiCompareString_NoCase>::ConstIterator it;
  if (xiiPathUtils::IsAbsolutePath(mainAsset))
  {
    it = m_ReferencedFiles.Find(mainAsset);
  }
  else
  {
    // Data dir parent relative?
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      xiiStringBuilder sDataDir;
      xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();
      sDataDir.PathParentDirectory();
      sDataDir.AppendPath(mainAsset);
      it = m_ReferencedFiles.Find(sDataDir);
      if (it.IsValid())
        break;
    }

    if (!it.IsValid())
    {
      // Data dir relative?
      for (const auto& dd : m_FileSystemConfig.m_DataDirs)
      {
        xiiStringBuilder sDataDir;
        xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();
        sDataDir.AppendPath(mainAsset);
        it = m_ReferencedFiles.Find(sDataDir);
        if (it.IsValid())
          break;
      }
    }
  }

  // Did we find an asset?
  if (it.IsValid() && it.Value().m_AssetGuid.IsValid())
  {
    xiiAssetInfo* pAssetInfo = nullptr;
    m_KnownAssets.TryGetValue(it.Value().m_AssetGuid, pAssetInfo);
    XII_ASSERT_DEV(pAssetInfo != nullptr, "Files reference non-existant assset!");

    if (subAsset.IsValid())
    {
      for (const xiiUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto itSub = m_KnownSubAssets.Find(sub);
        if (itSub.IsValid() && subAsset.IsEqual_NoCase(itSub.Value().GetName()))
        {
          return xiiLockedSubAsset(m_CuratorMutex, &itSub.Value());
        }
      }
    }
    else
    {
      auto itSub = m_KnownSubAssets.Find(pAssetInfo->m_Info->m_DocumentID);
      return xiiLockedSubAsset(m_CuratorMutex, &itSub.Value());
    }
  }

  if (!bExhaustiveSearch)
    return xiiLockedSubAsset();

  // TODO: This is the old slow code path that will find the longest substring match.
  // Should be removed or folded into FindBestMatchForFile once it's surely not needed anymore.

  auto FindAsset = [this](xiiStringView path) -> xiiAssetInfo* {
    // try to find the 'exact' relative path
    // otherwise find the shortest possible path
    xiiUInt32     uiMinLength = 0xFFFFFFFF;
    xiiAssetInfo* pBestInfo   = nullptr;

    if (path.IsEmpty())
      return nullptr;

    const xiiStringBuilder sPath = path;
    const xiiStringBuilder sPathWithSlash("/", sPath);

    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_sDataDirParentRelativePath.EndsWith_NoCase(sPath))
      {
        // endswith -> could also be equal
        if (path.IsEqual_NoCase(it.Value()->m_sDataDirParentRelativePath.GetData()))
        {
          // if equal, just take it
          return it.Value();
        }

        // need to check again with a slash to make sure we don't return something that is of an invalid type
        // this can happen where the user is allowed to type random paths
        if (it.Value()->m_sDataDirParentRelativePath.EndsWith_NoCase(sPathWithSlash))
        {
          const xiiUInt32 uiLength = it.Value()->m_sDataDirParentRelativePath.GetElementCount();
          if (uiLength < uiMinLength)
          {
            uiMinLength = uiLength;
            pBestInfo   = it.Value();
          }
        }
      }
    }

    return pBestInfo;
  };

  szSeparator = xiiStringUtils::FindSubString(szPathOrGuid, "|");
  if (szSeparator != nullptr)
  {
    xiiStringBuilder mainAsset2;
    mainAsset2.SetSubString_FromTo(szPathOrGuid, szSeparator);

    xiiStringView subAsset2(szSeparator + 1);
    if (xiiAssetInfo* pAssetInfo = FindAsset(mainAsset2))
    {
      for (const xiiUuid& sub : pAssetInfo->m_SubAssets)
      {
        auto subIt = m_KnownSubAssets.Find(sub);
        if (subIt.IsValid() && subAsset2.IsEqual_NoCase(subIt.Value().GetName()))
        {
          return xiiLockedSubAsset(m_CuratorMutex, &subIt.Value());
        }
      }
    }
  }

  xiiStringBuilder sPath = szPathOrGuid;
  sPath.MakeCleanPath();
  if (sPath.IsAbsolutePath())
  {
    if (!xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sPath))
      return xiiLockedSubAsset();
  }

  if (xiiAssetInfo* pAssetInfo = FindAsset(sPath))
  {
    auto itSub = m_KnownSubAssets.Find(pAssetInfo->m_Info->m_DocumentID);
    return xiiLockedSubAsset(m_CuratorMutex, &itSub.Value());
  }
  return xiiLockedSubAsset();
}

const xiiAssetCurator::xiiLockedSubAsset xiiAssetCurator::GetSubAsset(const xiiUuid& assetGuid) const
{
  XII_LOCK(m_CuratorMutex);

  auto it = m_KnownSubAssets.Find(assetGuid);
  if (it.IsValid())
  {
    const xiiSubAsset* pAssetInfo = &(it.Value());
    return xiiLockedSubAsset(m_CuratorMutex, pAssetInfo);
  }
  return xiiLockedSubAsset();
}

const xiiAssetCurator::xiiLockedSubAssetTable xiiAssetCurator::GetKnownSubAssets() const
{
  return xiiLockedSubAssetTable(m_CuratorMutex, &m_KnownSubAssets);
}

xiiUInt64 xiiAssetCurator::GetAssetDependencyHash(xiiUuid assetGuid)
{
  xiiUInt64 assetHash = 0;
  xiiUInt64 thumbHash = 0;
  xiiAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, false);
  return assetHash;
}

xiiUInt64 xiiAssetCurator::GetAssetReferenceHash(xiiUuid assetGuid)
{
  xiiUInt64 assetHash = 0;
  xiiUInt64 thumbHash = 0;
  xiiAssetCurator::UpdateAssetTransformState(assetGuid, assetHash, thumbHash, false);
  return thumbHash;
}

void xiiAssetCurator::GenerateTransitiveHull(const xiiStringView assetOrPath, xiiSet<xiiString>* pDependencies, xiiSet<xiiString>* pReferences)
{
  if (xiiConversionUtils::IsStringUuid(assetOrPath))
  {
    auto          it         = m_KnownSubAssets.Find(xiiConversionUtils::ConvertStringToUuid(assetOrPath));
    xiiAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    const bool    bInsertDep = pDependencies && !pDependencies->Contains(pAssetInfo->m_sAbsolutePath);
    const bool    bInsertRef = pReferences && !pReferences->Contains(pAssetInfo->m_sAbsolutePath);

    if (bInsertDep)
    {
      pDependencies->Insert(pAssetInfo->m_sAbsolutePath);
    }
    if (bInsertRef)
    {
      pReferences->Insert(pAssetInfo->m_sAbsolutePath);
    }

    if (pDependencies)
    {
      for (const xiiString& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
      {
        GenerateTransitiveHull(dep, pDependencies, nullptr);
      }
    }

    if (pReferences)
    {
      for (const xiiString& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
      {
        GenerateTransitiveHull(ref, nullptr, pReferences);
      }
    }
  }
  else
  {
    if (pDependencies && !pDependencies->Contains(assetOrPath))
    {
      pDependencies->Insert(assetOrPath);
    }
    if (pReferences && !pReferences->Contains(assetOrPath))
    {
      pReferences->Insert(assetOrPath);
    }
  }
}

xiiAssetInfo::TransformState xiiAssetCurator::IsAssetUpToDate(const xiiUuid& assetGuid, const xiiPlatformProfile*, const xiiAssetDocumentTypeDescriptor* pTypeDescriptor, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, bool bForce)
{
  return xiiAssetCurator::UpdateAssetTransformState(assetGuid, out_AssetHash, out_ThumbHash, bForce);
}

xiiAssetInfo::TransformState xiiAssetCurator::UpdateAssetTransformState(xiiUuid assetGuid, xiiUInt64& out_AssetHash, xiiUInt64& out_ThumbHash, bool bForce)
{
  CURATOR_PROFILE("UpdateAssetTransformState");
  {
    XII_LOCK(m_CuratorMutex);
    // If assetGuid is a sub-asset, redirect to main asset.
    auto it = m_KnownSubAssets.Find(assetGuid);
    if (!it.IsValid())
    {
      return xiiAssetInfo::Unknown;
    }
    xiiAssetInfo* pAssetInfo = it.Value().m_pAssetInfo;
    assetGuid                = pAssetInfo->m_Info->m_DocumentID;

    // Setting an asset to unknown actually does not change the m_TransformState but merely adds it to the m_TransformStateStale list.
    // This is to prevent the user facing state to constantly fluctuate if something is tagged as modified but not actually changed (E.g. saving a
    // file without modifying the content). Thus we need to check for m_TransformStateStale as well as for the set state.
    if (!bForce && pAssetInfo->m_TransformState != xiiAssetInfo::Unknown && !m_TransformStateStale.Contains(assetGuid))
    {
      out_AssetHash = pAssetInfo->m_AssetHash;
      out_ThumbHash = pAssetInfo->m_ThumbHash;
      return pAssetInfo->m_TransformState;
    }
  }
  if (EnsureAssetInfoUpdated(assetGuid).Failed())
  {
    xiiStringBuilder tmp;
    xiiLog::Error("Asset with GUID {0} is unknown", xiiConversionUtils::ToString(assetGuid, tmp));
    return xiiAssetInfo::TransformState::Unknown;
  }

  // Data to pull from the asset under the lock that is needed for update computation.
  xiiAssetDocumentManager*              pManager        = nullptr;
  const xiiAssetDocumentTypeDescriptor* pTypeDescriptor = nullptr;
  xiiString                             sAssetFile;
  xiiUInt8                              uiLastStateUpdate = 0;
  xiiUInt64                             uiSettingsHash    = 0;
  xiiHybridArray<xiiString, 16>         assetTransformDependencies;
  xiiHybridArray<xiiString, 16>         runtimeDependencies;
  xiiHybridArray<xiiString, 16>         outputs;

  // Lock asset and get all data needed for update computation.
  {
    CURATOR_PROFILE("CopyAssetData");
    XII_LOCK(m_CuratorMutex);
    xiiAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);

    pManager          = pAssetInfo->GetManager();
    pTypeDescriptor   = pAssetInfo->m_pDocumentTypeDescriptor;
    sAssetFile        = pAssetInfo->m_sAbsolutePath;
    uiLastStateUpdate = pAssetInfo->m_LastStateUpdate;
    // The settings has combines both the file settings and the global profile settings.
    uiSettingsHash = pAssetInfo->m_Info->m_uiSettingsHash + pManager->GetAssetProfileHash();
    for (const xiiString& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
    {
      assetTransformDependencies.PushBack(dep);
    }
    for (const xiiString& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
    {
      runtimeDependencies.PushBack(ref);
    }
    for (const xiiString& output : pAssetInfo->m_Info->m_Outputs)
    {
      outputs.PushBack(output);
    }
  }

  xiiAssetInfo::TransformState state = xiiAssetInfo::TransformState::Unknown;
  xiiSet<xiiString>            missingDependencies;
  xiiSet<xiiString>            missingReferences;
  // Compute final state and hashes.
  {
    state = HashAsset(uiSettingsHash, assetTransformDependencies, runtimeDependencies, missingDependencies, missingReferences, out_AssetHash, out_ThumbHash, bForce);
    XII_ASSERT_DEV(state == xiiAssetInfo::Unknown || state == xiiAssetInfo::MissingDependency || state == xiiAssetInfo::MissingReference, "Unhandled case of HashAsset return value.");

    if (state == xiiAssetInfo::Unknown)
    {
      if (pManager->IsOutputUpToDate(sAssetFile, outputs, out_AssetHash, pTypeDescriptor))
      {
        state = xiiAssetInfo::TransformState::UpToDate;
        if (pTypeDescriptor->m_AssetDocumentFlags.IsSet(xiiAssetDocumentFlags::SupportsThumbnail))
        {
          if (!xiiAssetDocumentManager::IsThumbnailUpToDate(sAssetFile, out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = xiiAssetInfo::TransformState::NeedsThumbnail;
          }
        }
        else if (pTypeDescriptor->m_AssetDocumentFlags.IsSet(xiiAssetDocumentFlags::AutoThumbnailOnTransform))
        {
          if (!xiiAssetDocumentManager::IsThumbnailUpToDate(sAssetFile, out_ThumbHash, pTypeDescriptor->m_pDocumentType->GetTypeVersion()))
          {
            state = xiiAssetInfo::TransformState::NeedsTransform;
          }
        }
      }
      else
      {
        state = xiiAssetInfo::TransformState::NeedsTransform;
      }
    }
  }

  {
    XII_LOCK(m_CuratorMutex);
    xiiAssetInfo* pAssetInfo = GetAssetInfo(assetGuid);
    if (pAssetInfo)
    {
      // Only update the state if the asset state remains unchanged since we gathered its data.
      // Otherwise the state we computed would already be stale. Return the data regardless
      // instead of waiting for a new computation as the case in which the value has actually changed
      // is very rare (asset modified between the two locks) in which case we will just create
      // an already stale transform / thumbnail which will be immediately replaced again.
      if (pAssetInfo->m_LastStateUpdate == uiLastStateUpdate)
      {
        UpdateAssetTransformState(assetGuid, state);
        pAssetInfo->m_AssetHash           = out_AssetHash;
        pAssetInfo->m_ThumbHash           = out_ThumbHash;
        pAssetInfo->m_MissingDependencies = std::move(missingDependencies);
        pAssetInfo->m_MissingReferences   = std::move(missingReferences);
        if (state == xiiAssetInfo::TransformState::UpToDate)
        {
          UpdateSubAssets(*pAssetInfo);
        }
      }
    }
    else
    {
      xiiStringBuilder tmp;
      xiiLog::Error("Asset with GUID {0} is unknown", xiiConversionUtils::ToString(assetGuid, tmp));
      return xiiAssetInfo::TransformState::Unknown;
    }
    return state;
  }
}

void xiiAssetCurator::GetAssetTransformStats(xiiUInt32& out_uiNumAssets, xiiHybridArray<xiiUInt32, xiiAssetInfo::TransformState::COUNT>& out_count)
{
  XII_LOCK(m_CuratorMutex);
  out_count.SetCountUninitialized(xiiAssetInfo::TransformState::COUNT);
  for (int i = 0; i < xiiAssetInfo::TransformState::COUNT; i++)
  {
    out_count[i] = m_TransformState[i].GetCount();
  }

  out_uiNumAssets = m_KnownAssets.GetCount();
}

xiiString xiiAssetCurator::FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const
{
  xiiStringBuilder sAssetPath(szAbsoluteAssetPath);

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    xiiStringBuilder sDataDir;
    xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    if (sAssetPath.IsPathBelowFolder(sDataDir))
      return sDataDir;
  }

  XII_REPORT_FAILURE("Could not find data directory for asset '{0}", szAbsoluteAssetPath);
  return xiiFileSystem::GetSdkRootDirectory();
}

xiiResult xiiAssetCurator::FindBestMatchForFile(xiiStringBuilder& sFile, xiiArrayPtr<xiiString> AllowedFileExtensions) const
{
  // TODO: Merge with exhaustive search in FindSubAsset
  sFile.MakeCleanPath();

  xiiStringBuilder testName = sFile;

  for (const auto& ext : AllowedFileExtensions)
  {
    testName.ChangeFileExtension(ext);

    if (xiiFileSystem::ExistsFile(testName))
    {
      sFile = testName;
      goto found;
    }
  }

  testName = sFile.GetFileNameAndExtension();

  if (testName.IsEmpty())
  {
    sFile = "";
    return XII_FAILURE;
  }

  if (xiiPathUtils::ContainsInvalidFilenameChars(testName))
  {
    // not much we can do here, if the filename is already invalid, we will probably not find it in out known files list

    xiiPathUtils::MakeValidFilename(testName, '_', sFile);
    return XII_FAILURE;
  }

  {
    XII_LOCK(m_CuratorMutex);

    auto SearchFile = [this](xiiStringBuilder& name) -> bool {
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status != xiiFileStatus::Status::Valid)
          continue;

        const xiiString& key = it.Key();

        if (key.EndsWith_NoCase(name))
        {
          name = it.Key();
          return true;
        }
      }

      return false;
    };

    // search for the full name
    {
      testName.Prepend("/"); // make sure to not find partial names

      for (const auto& ext : AllowedFileExtensions)
      {
        testName.ChangeFileExtension(ext);

        if (SearchFile(testName))
          goto found;
      }
    }

    return XII_FAILURE;
  }

found:
  if (xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(testName))
  {
    sFile = testName;
    return XII_SUCCESS;
  }

  return XII_FAILURE;
}

void xiiAssetCurator::FindAllUses(xiiUuid assetGuid, xiiSet<xiiUuid>& uses, bool transitive) const
{
  XII_LOCK(m_CuratorMutex);

  xiiSet<xiiUuid> todoList;
  todoList.Insert(assetGuid);

  auto GatherReferences = [&](const xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>>& inverseTracker, const xiiStringBuilder& sAsset) {
    auto it = inverseTracker.Find(sAsset);
    if (it.IsValid())
    {
      for (const xiiUuid& guid : it.Value())
      {
        if (!uses.Contains(guid))
          todoList.Insert(guid);

        uses.Insert(guid);
      }
    }
  };

  xiiStringBuilder sCurrentAsset;
  do
  {
    auto                itFirst = todoList.GetIterator();
    const xiiAssetInfo* pInfo   = GetAssetInfo(itFirst.Key());
    todoList.Remove(itFirst);

    if (pInfo)
    {
      sCurrentAsset = pInfo->m_sAbsolutePath;
      GatherReferences(m_InverseReferences, sCurrentAsset);
      GatherReferences(m_InverseDependency, sCurrentAsset);
    }
  } while (transitive && !todoList.IsEmpty());
}

////////////////////////////////////////////////////////////////////////
// xiiAssetCurator Manual and Automatic Change Notification
////////////////////////////////////////////////////////////////////////

void xiiAssetCurator::NotifyOfFileChange(const char* szAbsolutePath)
{
  xiiStringBuilder sPath(szAbsolutePath);
  sPath.MakeCleanPath();
  HandleSingleFile(sPath);
  // MainThreadTick();
}

void xiiAssetCurator::NotifyOfAssetChange(const xiiUuid& assetGuid)
{
  InvalidateAssetTransformState(assetGuid);
}

void xiiAssetCurator::UpdateAssetLastAccessTime(const xiiUuid& assetGuid)
{
  auto it = m_KnownSubAssets.Find(assetGuid);

  if (!it.IsValid())
    return;

  it.Value().m_LastAccess = xiiTime::Now();
}

void xiiAssetCurator::CheckFileSystem()
{
  XII_PROFILE_SCOPE("CheckFileSystem");
  xiiStopwatch sw;

  xiiProgressRange* range = nullptr;
  if (xiiThreadUtils::IsMainThread())
    range = XII_DEFAULT_NEW(xiiProgressRange, "Check File-System for Assets", m_FileSystemConfig.m_DataDirs.GetCount(), false);

  // make sure the hashing task has finished
  ShutdownUpdateTask();

  XII_LOCK(m_CuratorMutex);

  SetAllAssetStatusUnknown();

  // check every data directory
  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    xiiStringBuilder sTemp;
    xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).IgnoreResult();

    if (xiiThreadUtils::IsMainThread())
      range->BeginNextStep(dd.m_sDataDirSpecialPath);

    IterateDataDirectory(sTemp);
  }

  RemoveStaleFileInfos();

  if (xiiThreadUtils::IsMainThread())
  {
    XII_DEFAULT_DELETE(range);
    // Broadcast reset only if we are on the main thread.
    // Otherwise we are on the init task thread and the reset will be called on the main thread by WaitForInitialize.
    xiiAssetCuratorEvent e;
    e.m_pInfo = nullptr;
    e.m_Type  = xiiAssetCuratorEvent::Type::AssetListReset;
    m_Events.Broadcast(e);
  }

  RestartUpdateTask();

  xiiLog::Debug("Asset Curator Refresh Time: {0} ms", xiiArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void xiiAssetCurator::NeedsReloadResources()
{
  if (m_bNeedToReloadResources)
    return;

  m_bNeedToReloadResources = true;
  m_NextReloadResources    = xiiTime::Now() + xiiTime::Seconds(1.5);
}

////////////////////////////////////////////////////////////////////////
// xiiAssetCurator Processing
////////////////////////////////////////////////////////////////////////

xiiCommandLineOptionEnum opt_AssetThumbnails("_Editor", "-AssetThumbnails", "Whether to generate thumbnails for transformed assets.", "default = 0 | never = 1", 0);

xiiTransformStatus xiiAssetCurator::ProcessAsset(xiiAssetInfo* pAssetInfo, const xiiPlatformProfile* pAssetProfile, xiiBitflags<xiiTransformFlags> transformFlags)
{
  if (transformFlags.IsSet(xiiTransformFlags::ForceTransform))
    xiiLog::Dev("Asset transform forced.");

  const xiiAssetDocumentTypeDescriptor* pTypeDesc   = pAssetInfo->m_pDocumentTypeDescriptor;
  xiiUInt64                             uiHash      = 0;
  xiiUInt64                             uiThumbHash = 0;
  xiiAssetInfo::TransformState          state       = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash);

  for (const auto& dep : pAssetInfo->m_Info->m_AssetTransformDependencies)
  {
    xiiBitflags<xiiTransformFlags> transformFlagsDeps = transformFlags;
    transformFlagsDeps.Remove(xiiTransformFlags::ForceTransform);
    if (xiiAssetInfo* pInfo = GetAssetInfo(dep))
    {
      XII_SUCCEED_OR_RETURN(ProcessAsset(pInfo, pAssetProfile, transformFlagsDeps));
    }
  }

  xiiTransformStatus resReferences;
  for (const auto& ref : pAssetInfo->m_Info->m_RuntimeDependencies)
  {
    xiiBitflags<xiiTransformFlags> transformFlagsRefs = transformFlags;
    transformFlagsRefs.Remove(xiiTransformFlags::ForceTransform);
    if (xiiAssetInfo* pInfo = GetAssetInfo(ref))
    {
      resReferences = ProcessAsset(pInfo, pAssetProfile, transformFlagsRefs);
      if (resReferences.Failed())
        break;
    }
  }


  XII_ASSERT_DEV(pTypeDesc->m_pDocumentType->IsDerivedFrom<xiiAssetDocument>(), "Asset document does not derive from correct base class ('{0}')", pAssetInfo->m_sDataDirParentRelativePath);

  auto assetFlags = pTypeDesc->m_AssetDocumentFlags;

  // Skip assets that cannot be auto-transformed.
  {
    if (assetFlags.IsAnySet(xiiAssetDocumentFlags::DisableTransform))
      return xiiStatus(XII_SUCCESS);

    if (!transformFlags.IsSet(xiiTransformFlags::TriggeredManually) && assetFlags.IsAnySet(xiiAssetDocumentFlags::OnlyTransformManually))
      return xiiStatus(XII_SUCCESS);
  }

  // If references are not complete and we generate thumbnails on transform we can cancel right away.
  if (assetFlags.IsSet(xiiAssetDocumentFlags::AutoThumbnailOnTransform) && resReferences.Failed())
  {
    return resReferences;
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  {
    // Sanity check that transforming the dependencies did not change the asset's transform state.
    // In theory this can happen if an asset is transformed by multiple processes at the same time or changes to the file system are being made in the middle of the transform.
    // If this can be reproduced consistently, it is usually a bug in the dependency tracking or other part of the asset curator.
    xiiUInt64                    uiHash2      = 0;
    xiiUInt64                    uiThumbHash2 = 0;
    xiiAssetInfo::TransformState state2       = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash2, uiThumbHash2);

    if (uiHash != uiHash2)
      return xiiTransformStatus(xiiFmt("Asset hash changed while prosessing dependencies from {} to {}", uiHash, uiHash2));
    if (uiThumbHash != uiThumbHash2)
      return xiiTransformStatus(xiiFmt("Asset thumbnail hash changed while prosessing dependencies from {} to {}", uiThumbHash, uiThumbHash2));
    if (state != state2)
      return xiiTransformStatus(xiiFmt("Asset state changed while prosessing dependencies from {} to {}", state, state2));
  }
#endif

  if (transformFlags.IsSet(xiiTransformFlags::ForceTransform))
  {
    state = xiiAssetInfo::NeedsTransform;
  }

  if (state == xiiAssetInfo::TransformState::UpToDate)
    return xiiStatus(XII_SUCCESS);

  if (state == xiiAssetInfo::TransformState::MissingDependency)
  {
    return xiiTransformStatus(xiiFmt("Missing dependency for asset '{0}', can't transform.", pAssetInfo->m_sAbsolutePath));
  }

  // does the document already exist and is open ?
  bool         bWasOpen = false;
  xiiDocument* pDoc     = pTypeDesc->m_pManager->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = xiiQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath, xiiDocumentFlags::None);

  if (pDoc == nullptr)
    return xiiTransformStatus(xiiFmt("Could not open asset document '{0}'", pAssetInfo->m_sDataDirParentRelativePath));

  XII_SCOPE_EXIT(if (!pDoc->HasWindowBeenRequested() && !bWasOpen) pDoc->GetDocumentManager()->CloseDocument(pDoc););

  xiiTransformStatus ret;
  xiiAssetDocument*  pAsset = static_cast<xiiAssetDocument*>(pDoc);
  if (state == xiiAssetInfo::TransformState::NeedsTransform || (state == xiiAssetInfo::TransformState::NeedsThumbnail && assetFlags.IsSet(xiiAssetDocumentFlags::AutoThumbnailOnTransform)) || (transformFlags.IsSet(xiiTransformFlags::TriggeredManually) && state == xiiAssetInfo::TransformState::NeedsImport))
  {
    ret = pAsset->TransformAsset(transformFlags, pAssetProfile);
  }

  if (state == xiiAssetInfo::TransformState::MissingReference)
  {
    return xiiTransformStatus(xiiFmt("Missing reference for asset '{0}', can't create thumbnail.", pAssetInfo->m_sAbsolutePath));
  }

  if (opt_AssetThumbnails.GetOptionValue(xiiCommandLineOption::LogMode::FirstTimeIfSpecified) != 1)
  {
    // skip thumbnail generation, if disabled globally

    if (ret.Succeeded() && assetFlags.IsSet(xiiAssetDocumentFlags::SupportsThumbnail) && !assetFlags.IsSet(xiiAssetDocumentFlags::AutoThumbnailOnTransform) && !resReferences.Failed())
    {
      // If the transformed succeeded, the asset should now be in the NeedsThumbnail state unless the thumbnail already exists in which case we are done or the transform made changes to the asset, e.g. a mesh imported new materials in which case we will revert to transform needed as our dependencies need transform. We simply skip the thumbnail generation in this case.
      xiiAssetInfo::TransformState state3 = IsAssetUpToDate(pAssetInfo->m_Info->m_DocumentID, pAssetProfile, pTypeDesc, uiHash, uiThumbHash);
      if (state3 == xiiAssetInfo::TransformState::NeedsThumbnail)
      {
        ret = pAsset->CreateThumbnail();
      }
    }
  }

  return ret;
}


xiiStatus xiiAssetCurator::ResaveAsset(xiiAssetInfo* pAssetInfo)
{
  bool         bWasOpen = false;
  xiiDocument* pDoc     = pAssetInfo->GetManager()->GetDocumentByPath(pAssetInfo->m_sAbsolutePath);
  if (pDoc)
    bWasOpen = true;
  else
    pDoc = xiiQtEditorApp::GetSingleton()->OpenDocument(pAssetInfo->m_sAbsolutePath, xiiDocumentFlags::None);

  if (pDoc == nullptr)
    return xiiStatus(xiiFmt("Could not open asset document '{0}'", pAssetInfo->m_sDataDirParentRelativePath));

  xiiStatus ret = pDoc->SaveDocument(true);

  if (!pDoc->HasWindowBeenRequested() && !bWasOpen)
    pDoc->GetDocumentManager()->CloseDocument(pDoc);

  return ret;
}

xiiAssetInfo* xiiAssetCurator::GetAssetInfo(const xiiUuid& assetGuid)
{
  xiiAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;
  return nullptr;
}

const xiiAssetInfo* xiiAssetCurator::GetAssetInfo(const xiiUuid& assetGuid) const
{
  xiiAssetInfo* pAssetInfo = nullptr;
  if (m_KnownAssets.TryGetValue(assetGuid, pAssetInfo))
    return pAssetInfo;
  return nullptr;
}

xiiAssetInfo* xiiAssetCurator::GetAssetInfo(const xiiString& sAssetGuid)
{
  if (sAssetGuid.IsEmpty())
    return nullptr;

  if (xiiConversionUtils::IsStringUuid(sAssetGuid))
  {
    const xiiUuid guid = xiiConversionUtils::ConvertStringToUuid(sAssetGuid);

    xiiAssetInfo* pInfo = nullptr;
    if (m_KnownAssets.TryGetValue(guid, pInfo))
      return pInfo;
  }

  return nullptr;
}

xiiSubAsset* xiiAssetCurator::GetSubAssetInternal(const xiiUuid& assetGuid)
{
  auto it = m_KnownSubAssets.Find(assetGuid);

  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

void xiiAssetCurator::HandleSingleFile(const xiiString& sAbsolutePath)
{
  CURATOR_PROFILE("HandleSingleFile");
  XII_LOCK(m_CuratorMutex);

  xiiFileStats Stats;
  if (xiiOSFile::GetFileStats(sAbsolutePath, Stats).Failed())
  {
    // this is a bit tricky:
    // when the document is deleted on disk, it would be nicer not to close it (discarding modifications!)
    // instead we could set it as modified
    // but then when it was only moved or renamed that means we have another document with the same GUID
    // so once the user would save the now modified document, we would end up with two documents with the same GUID
    // so, for now, since this is probably a rare case anyway, we just close the document without asking
    xiiDocumentManager::EnsureDocumentIsClosedInAllManagers(sAbsolutePath);

    if (xiiFileStatus* pFileStatus = m_ReferencedFiles.GetValue(sAbsolutePath))
    {
      pFileStatus->m_Timestamp.Invalidate();
      pFileStatus->m_uiHash = 0;
      pFileStatus->m_Status = xiiFileStatus::Status::Unknown;

      xiiUuid guid0 = pFileStatus->m_AssetGuid;
      if (guid0.IsValid())
      {
        xiiAssetInfo* pAssetInfo = m_KnownAssets[guid0];
        UntrackDependencies(pAssetInfo);
        RemoveAssetTransformState(guid0);
        SetAssetExistanceState(*pAssetInfo, xiiAssetExistanceState::FileRemoved);
        pFileStatus->m_AssetGuid = xiiUuid();
      }

      auto it = m_InverseDependency.Find(sAbsolutePath);
      if (it.IsValid())
      {
        for (const xiiUuid& guid : it.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }

      auto it2 = m_InverseReferences.Find(sAbsolutePath);
      if (it2.IsValid())
      {
        for (const xiiUuid& guid : it2.Value())
        {
          InvalidateAssetTransformState(guid);
        }
      }
    }

    return;
  }
  else
  {
    XII_ASSERT_DEV(!Stats.m_bIsDirectory, "Directories are handled by xiiAssetWatcher and should not pass into this function.");
  }

  HandleSingleFile(sAbsolutePath, Stats);
}

void xiiAssetCurator::HandleSingleFile(const xiiString& sAbsolutePath, const xiiFileStats& FileStat)
{
  XII_ASSERT_DEV(!FileStat.m_bIsDirectory, "Directories are handled by xiiAssetWatcher and should not pass into this function.");
  CURATOR_PROFILE("HandleSingleFile2");
  XII_LOCK(m_CuratorMutex);

  xiiStringBuilder sExt = xiiPathUtils::GetFileExtension(sAbsolutePath);
  sExt.ToLower();

  // store information for every file, even when it is no asset, it might be a dependency for some asset
  auto& RefFile = m_ReferencedFiles[sAbsolutePath];

  // mark the file as valid (i.e. we saw it on disk, so it hasn't been deleted or such)
  RefFile.m_Status = xiiFileStatus::Status::Valid;

  bool fileChanged = !RefFile.m_Timestamp.Compare(FileStat.m_LastModificationTime, xiiTimestamp::CompareMode::Identical);
  if (fileChanged)
  {
    RefFile.m_Timestamp.Invalidate();
    RefFile.m_uiHash = 0;
    if (RefFile.m_AssetGuid.IsValid())
      InvalidateAssetTransformState(RefFile.m_AssetGuid);

    auto it = m_InverseDependency.Find(sAbsolutePath);
    if (it.IsValid())
    {
      for (const xiiUuid& guid : it.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }

    auto it2 = m_InverseReferences.Find(sAbsolutePath);
    if (it2.IsValid())
    {
      for (const xiiUuid& guid : it2.Value())
      {
        InvalidateAssetTransformState(guid);
      }
    }
  }

  // Assets should never be in an AssetCache folder.
  const char* szNeedle = sAbsolutePath.FindSubString("AssetCache/");
  if (szNeedle != nullptr && sAbsolutePath.GetData() != szNeedle && szNeedle[-1] == '/')
  {
    return;
  }

  // check that this is an asset type that we know
  if (!m_ValidAssetExtensions.Contains(sExt))
  {
    return;
  }

  // the file is a known asset type
  // so make sure it gets a valid GUID assigned

  // File hasn't change, early out.
  if (RefFile.m_AssetGuid.IsValid() && !fileChanged)
    return;

  // store the folder of the asset
  {
    xiiStringBuilder sAssetFolder = sAbsolutePath;
    sAssetFolder                  = sAssetFolder.GetFileDirectory();

    m_AssetFolders.Insert(sAssetFolder);
  }

  // This will update the timestamp for assets.
  EnsureAssetInfoUpdated(sAbsolutePath).IgnoreResult();
}

xiiResult xiiAssetCurator::WriteAssetTable(const char* szDataDirectory, const xiiPlatformProfile* pAssetProfile0 /*= nullptr*/)
{
  const xiiPlatformProfile* pAssetProfile = pAssetProfile0;

  if (pAssetProfile == nullptr)
  {
    pAssetProfile = GetActiveAssetProfile();
  }

  xiiStringBuilder sDataDir = szDataDirectory;
  sDataDir.MakeCleanPath();

  xiiStringBuilder sFinalPath(sDataDir, "/AssetCache/", pAssetProfile->GetConfigName(), ".xiiAidlt");
  sFinalPath.MakeCleanPath();

  xiiStringBuilder sTemp, sTemp2;
  xiiString        sResourcePath;

  xiiMap<xiiString, xiiString> GuidToPath;

  {
    for (auto& man : xiiAssetDocumentManager::GetAllDocumentManagers())
    {
      if (!man->GetDynamicRTTI()->IsDerivedFrom<xiiAssetDocumentManager>())
        continue;

      xiiAssetDocumentManager* pManager = static_cast<xiiAssetDocumentManager*>(man);

      // allow to add fully custom entries
      pManager->AddEntriesToAssetTable(sDataDir, pAssetProfile, GuidToPath);
    }
  }

  // TODO: Iterate over m_KnownSubAssets instead
  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    sTemp = it.Value()->m_sAbsolutePath;

    // ignore all assets that are not located in this data directory
    if (!sTemp.IsPathBelowFolder(sDataDir))
      continue;

    xiiAssetDocumentManager* pManager = it.Value()->GetManager();

    auto WriteEntry = [this, &sDataDir, &pAssetProfile, &GuidToPath, pManager, &sTemp, &sTemp2](const xiiUuid& guid) {
      xiiSubAsset* pSub   = GetSubAssetInternal(guid);
      xiiString    sEntry = pManager->GetAssetTableEntry(pSub, sDataDir, pAssetProfile);

      // it is valid to write no asset table entry, if no redirection is required
      // this is used by decal assets for instance
      if (!sEntry.IsEmpty())
      {
        xiiConversionUtils::ToString(guid, sTemp2);

        GuidToPath[sTemp2] = sEntry;
      }
    };

    WriteEntry(it.Key());
    for (const xiiUuid& subGuid : it.Value()->m_SubAssets)
    {
      WriteEntry(subGuid);
    }
  }

  xiiDeferredFileWriter file;
  file.SetOutput(sFinalPath);

  for (auto it = GuidToPath.GetIterator(); it.IsValid(); ++it)
  {
    const xiiString& guid = it.Key();
    const xiiString& path = it.Value();

    file.WriteBytes(guid.GetData(), guid.GetElementCount()).IgnoreResult();
    file.WriteBytes(";", 1).IgnoreResult();
    file.WriteBytes(path.GetData(), path.GetElementCount()).IgnoreResult();
    file.WriteBytes("\n", 1).IgnoreResult();
  }

  if (file.Close().Failed())
  {
    xiiLog::Error("Failed to open asset lookup table file ('{0}')", sFinalPath);
    return XII_FAILURE;
  }

  return XII_SUCCESS;
}

void xiiAssetCurator::ProcessAllCoreAssets()
{
  XII_PROFILE_SCOPE("ProcessAllCoreAssets");
  if (xiiQtUiServices::IsHeadless())
    return;

  // The 'Core Assets' are always transformed for the PC platform,
  // as they are needed to run the editor properly
  const xiiPlatformProfile* pAssetProfile = GetDevelopmentAssetProfile();

  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    xiiStringBuilder sCoreCollectionPath;
    xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sCoreCollectionPath).IgnoreResult();

    xiiStringBuilder sName = sCoreCollectionPath.GetFileName();
    sName.Append(".xiiCollectionAsset");
    sCoreCollectionPath.AppendPath(sName);

    QFile coreCollection(sCoreCollectionPath.GetData());
    if (coreCollection.exists())
    {
      auto pSubAsset = FindSubAsset(sCoreCollectionPath);
      if (pSubAsset)
      {
        // prefer certain asset types over others, to ensure that thumbnail generation works
        xiiHybridArray<xiiTempHashedString, 4> transformOrder;
        transformOrder.PushBack(xiiTempHashedString("RenderPipeline"));
        transformOrder.PushBack(xiiTempHashedString(""));

        xiiTransformStatus resReferences(XII_SUCCESS);

        for (const xiiTempHashedString& name : transformOrder)
        {
          for (const auto& ref : pSubAsset->m_pAssetInfo->m_Info->m_RuntimeDependencies)
          {
            if (xiiAssetInfo* pInfo = GetAssetInfo(ref))
            {
              if (name.GetHash() == 0ull || pInfo->m_Info->m_sAssetsDocumentTypeName == name)
              {
                resReferences = ProcessAsset(pInfo, pAssetProfile, xiiTransformFlags::TriggeredManually);
                if (resReferences.Failed())
                  break;
              }
            }
          }
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
// xiiAssetCurator Update Task
////////////////////////////////////////////////////////////////////////

void xiiAssetCurator::RestartUpdateTask()
{
  XII_LOCK(m_CuratorMutex);
  m_bRunUpdateTask = true;

  RunNextUpdateTask();
}

void xiiAssetCurator::ShutdownUpdateTask()
{
  {
    XII_LOCK(m_CuratorMutex);
    m_bRunUpdateTask = false;
  }

  if (m_pUpdateTask)
  {
    xiiTaskSystem::WaitForGroup(m_UpdateTaskGroup);

    XII_LOCK(m_CuratorMutex);
    m_pUpdateTask.Clear();
  }
}

bool xiiAssetCurator::GetNextAssetToUpdate(xiiUuid& guid, xiiStringBuilder& out_sAbsPath)
{
  XII_LOCK(m_CuratorMutex);

  while (!m_TransformStateStale.IsEmpty())
  {
    auto it = m_TransformStateStale.GetIterator();
    guid    = it.Key();

    auto pAssetInfo = GetAssetInfo(guid);

    // XII_ASSERT_DEBUG(pAssetInfo != nullptr, "Non-existent assets should not have a tracked transform state.");

    if (pAssetInfo != nullptr)
    {
      out_sAbsPath = pAssetInfo->m_sAbsolutePath;
      return true;
    }
    else
    {
      xiiLog::Error("Non-existent assets ('{0}') should not have a tracked transform state.", guid);
      m_TransformStateStale.Remove(it);
    }
  }

  return false;
}

void xiiAssetCurator::OnUpdateTaskFinished(const xiiSharedPtr<xiiTask>& pTask)
{
  XII_LOCK(m_CuratorMutex);

  RunNextUpdateTask();
}

void xiiAssetCurator::RunNextUpdateTask()
{
  XII_LOCK(m_CuratorMutex);

  if (!m_bRunUpdateTask || (m_TransformStateStale.IsEmpty() && m_TransformState[xiiAssetInfo::TransformState::Unknown].IsEmpty()))
    return;

  if (m_pUpdateTask == nullptr)
  {
    m_pUpdateTask = XII_DEFAULT_NEW(xiiUpdateTask, xiiMakeDelegate(&xiiAssetCurator::OnUpdateTaskFinished, this));
  }

  if (m_pUpdateTask->IsTaskFinished())
  {
    m_UpdateTaskGroup = xiiTaskSystem::StartSingleTask(m_pUpdateTask, xiiTaskPriority::FileAccess);
  }
}


////////////////////////////////////////////////////////////////////////
// xiiAssetCurator Check File System Helper
////////////////////////////////////////////////////////////////////////

void xiiAssetCurator::SetAllAssetStatusUnknown()
{
  // tags all known files as unknown, such that we can later remove files
  // that can not be found anymore

  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Status = xiiFileStatus::Status::Unknown;
  }

  for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
  {
    UpdateAssetTransformState(it.Key(), xiiAssetInfo::TransformState::Unknown);
  }
}

void xiiAssetCurator::RemoveStaleFileInfos()
{
  xiiSet<xiiString> unknownFiles;
  for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
  {
    // search for files that existed previously but have not been found anymore recently
    if (it.Value().m_Status == xiiFileStatus::Status::Unknown)
    {
      unknownFiles.Insert(it.Key());
    }
  }

  for (const xiiString& sFile : unknownFiles)
  {
    HandleSingleFile(sFile);
    m_ReferencedFiles.Remove(sFile);
  }
}

void xiiAssetCurator::BuildFileExtensionSet(xiiSet<xiiString>& AllExtensions)
{
  xiiStringBuilder sTemp;
  AllExtensions.Clear();

  const auto& assetTypes = xiiAssetDocumentManager::GetAllDocumentDescriptors();

  // use translated strings
  xiiMap<xiiString, const xiiDocumentTypeDescriptor*> allDesc;
  for (auto it : assetTypes)
  {
    allDesc[xiiTranslate(it.Key())] = it.Value();
  }

  for (auto it : allDesc)
  {
    const auto desc = it.Value();

    if (desc->m_pManager->GetDynamicRTTI()->IsDerivedFrom<xiiAssetDocumentManager>())
    {
      sTemp = desc->m_sFileExtension;
      sTemp.ToLower();

      AllExtensions.Insert(sTemp);
    }
  }
}

void xiiAssetCurator::IterateDataDirectory(const char* szDataDir, xiiSet<xiiString>* pFoundFiles)
{
  xiiStringBuilder sDataDir = szDataDir;
  sDataDir.MakeCleanPath();
  XII_ASSERT_DEV(xiiPathUtils::IsAbsolutePath(szDataDir), "Only absolute paths are supported for directory iteration.");

  while (sDataDir.EndsWith("/"))
    sDataDir.Shrink(0, 1);

  if (sDataDir.IsEmpty())
    return;

  xiiFileSystemIterator iterator;
  iterator.StartSearch(sDataDir, xiiFileSystemIteratorFlags::ReportFilesRecursive);

  if (!iterator.IsValid())
    return;

  xiiStringBuilder sPath;

  for (; iterator.IsValid(); iterator.Next())
  {
    sPath = iterator.GetCurrentPath();
    sPath.AppendPath(iterator.GetStats().m_sName);
    sPath.MakeCleanPath();

    HandleSingleFile(sPath, iterator.GetStats());
    if (pFoundFiles)
    {
      pFoundFiles->Insert(sPath);
    }
  }
}

void xiiAssetCurator::LoadCaches()
{
  XII_PROFILE_SCOPE("LoadCaches");
  XII_LOCK(m_CuratorMutex);

  xiiStopwatch sw;
  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    xiiStringBuilder sDataDir;
    xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    xiiStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.xiiCache");

    xiiFileReader reader;
    if (reader.Open(sCacheFile).Succeeded())
    {
      xiiUInt32 uiCuratorCacheVersion = 0;
      xiiUInt32 uiFileVersion         = 0;
      xiiUInt32 uiAssetCount          = 0;
      xiiUInt32 uiFileCount           = 0;
      reader >> uiCuratorCacheVersion;
      reader >> uiFileVersion;
      reader >> uiAssetCount;
      reader >> uiFileCount;

      m_KnownAssets.Reserve(m_CachedAssets.GetCount());
      m_KnownSubAssets.Reserve(m_CachedAssets.GetCount());

      m_TransformState[xiiAssetInfo::Unknown].Reserve(m_CachedAssets.GetCount());
      m_TransformState[xiiAssetInfo::UpToDate].Reserve(m_CachedAssets.GetCount());
      m_SubAssetChanged.Reserve(m_CachedAssets.GetCount());
      m_TransformStateStale.Reserve(m_CachedAssets.GetCount());
      m_Updating.Reserve(m_CachedAssets.GetCount());

      if (uiCuratorCacheVersion != XII_CURATOR_CACHE_VERSION)
      {
        // Do not purge cache on processors.
        if (!xiiQtUiServices::IsHeadless())
        {
          xiiStringBuilder sCacheDir = sDataDir;
          sCacheDir.AppendPath("AssetCache");

          QDir dir(sCacheDir.GetData());
          if (dir.exists())
          {
            dir.removeRecursively();
          }
        }
        continue;
      }

      if (uiFileVersion != XII_CURATOR_CACHE_FILE_VERSION)
        continue;

      const xiiRTTI* pFileStatusType = xiiGetStaticRTTI<xiiFileStatus>();
      {
        XII_PROFILE_SCOPE("Assets");
        for (xiiUInt32 i = 0; i < uiAssetCount; i++)
        {
          xiiString sPath;
          reader >> sPath;

          const xiiRTTI*        pType  = nullptr;
          xiiAssetDocumentInfo* pEntry = static_cast<xiiAssetDocumentInfo*>(xiiReflectionSerializer::ReadObjectFromBinary(reader, pType));
          XII_ASSERT_DEBUG(pEntry != nullptr && pType == xiiGetStaticRTTI<xiiAssetDocumentInfo>(), "Failed to deserialize xiiAssetDocumentInfo!");
          m_CachedAssets.Insert(sPath, xiiUniquePtr<xiiAssetDocumentInfo>(pEntry, xiiFoundation::GetDefaultAllocator()));

          xiiFileStatus stat;
          reader >> stat;
          m_CachedFiles.Insert(std::move(sPath), stat);
        }
      }
      {
        XII_PROFILE_SCOPE("Files");
        for (xiiUInt32 i = 0; i < uiFileCount; i++)
        {
          xiiString sPath;
          reader >> sPath;
          xiiFileStatus stat;
          reader >> stat;
          m_ReferencedFiles.Insert(std::move(sPath), stat);
        }
      }
    }
  }

  xiiLog::Debug("Asset Curator LoadCaches: {0} ms", xiiArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}

void xiiAssetCurator::SaveCaches()
{
  XII_PROFILE_SCOPE("SaveCaches");
  m_CachedAssets.Clear();
  m_CachedFiles.Clear();

  // Do not save cache on processors.
  if (xiiQtUiServices::IsHeadless())
    return;

  XII_LOCK(m_CuratorMutex);
  const xiiUInt32 uiCuratorCacheVersion = XII_CURATOR_CACHE_VERSION;

  xiiStopwatch sw;
  for (const auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    xiiStringBuilder sDataDir;
    xiiFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sDataDir).IgnoreResult();

    xiiStringBuilder sCacheFile = sDataDir;
    sCacheFile.AppendPath("AssetCache", "AssetCurator.xiiCache");

    const xiiUInt32 uiFileVersion = XII_CURATOR_CACHE_FILE_VERSION;
    xiiUInt32       uiAssetCount  = 0;
    xiiUInt32       uiFileCount   = 0;

    {
      XII_PROFILE_SCOPE("Count");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value()->m_ExistanceState == xiiAssetExistanceState::FileUnchanged && it.Value()->m_sAbsolutePath.StartsWith(sDataDir))
        {
          ++uiAssetCount;
        }
      }
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Value().m_Status == xiiFileStatus::Status::Valid && !it.Value().m_AssetGuid.IsValid() && it.Key().StartsWith(sDataDir))
        {
          ++uiFileCount;
        }
      }
    }
    xiiDeferredFileWriter writer;
    writer.SetOutput(sCacheFile);

    writer << uiCuratorCacheVersion;
    writer << uiFileVersion;
    writer << uiAssetCount;
    writer << uiFileCount;

    {
      XII_PROFILE_SCOPE("Assets");
      for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
      {
        const xiiAssetInfo* pAsset = it.Value();
        if (pAsset->m_ExistanceState == xiiAssetExistanceState::FileUnchanged && pAsset->m_sAbsolutePath.StartsWith(sDataDir))
        {
          writer << pAsset->m_sAbsolutePath;
          xiiReflectionSerializer::WriteObjectToBinary(writer, xiiGetStaticRTTI<xiiAssetDocumentInfo>(), pAsset->m_Info.Borrow());
          const xiiFileStatus* pStat = m_ReferencedFiles.GetValue(it.Value()->m_sAbsolutePath);
          XII_ASSERT_DEBUG(pStat != nullptr, "");
          writer << *pStat;
        }
      }
    }
    {
      XII_PROFILE_SCOPE("Files");
      for (auto it = m_ReferencedFiles.GetIterator(); it.IsValid(); ++it)
      {
        const xiiFileStatus& stat = it.Value();
        if (stat.m_Status == xiiFileStatus::Status::Valid && !stat.m_AssetGuid.IsValid() && it.Key().StartsWith(sDataDir))
        {
          writer << it.Key();
          writer << stat;
        }
      }
    }
    writer.Close().IgnoreResult();
  }

  xiiLog::Debug("Asset Curator SaveCaches: {0} ms", xiiArgF(sw.GetRunningTotal().GetMilliseconds(), 3));
}
