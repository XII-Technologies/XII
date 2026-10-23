/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

xiiStatus xiiAssetProcessor::QueueTransformAssets(xiiArrayPtr<const xiiUuid> assets, xiiBitflags<xiiTransformFlags> flags)
{
  return QueueTransforms(assets, flags, false);
}

xiiStatus xiiAssetProcessor::QueueTransforms(xiiArrayPtr<const xiiUuid> assets, xiiBitflags<xiiTransformFlags> flags, bool bAllAssets)
{
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "Save and queue transformations on the editor thread.");
  if (!xiiToolsProject::IsProjectOpen() || xiiToolsProject::IsProjectClosing())
    return xiiStatus("No project is available for transformation.");

  auto*                       curator = xiiAssetCurator::GetSingleton();
  xiiSet<xiiTempHashedString> finalTypes;
  if (bAllAssets)
  {
    for (auto* manager : xiiDocumentManager::GetAllDocumentManagers())
      if (auto* assetManager = xiiDynamicCast<xiiAssetDocumentManager*>(manager))
        assetManager->GetAssetTypesRequiringTransformForSceneExport(finalTypes);
  }
  // Save documents only on the GUI thread. The subprocess reads saved inputs and never
  // modifies the editor's live document graph. Imports retain their NeedsImport result.
  for (const auto& guid : assets)
  {
    if (auto* document = xiiDocumentManager::GetDocumentByGuid(guid); flags.IsSet(xiiTransformFlags::TriggeredManually) && document && document->IsModified())
      XII_SUCCEED_OR_RETURN(document->SaveDocument());
  }
  {
    XII_LOCK(m_ProcessorMutex);
    if (m_ProcessTaskState == ProcessTaskState::Stopping)
      return xiiStatus("Asset processing is stopping. Retry once it has stopped.");
    XII_LOCK(curator->m_CuratorMutex);
    if (!m_TransformProgress.m_bActive)
    {
      m_TransformProgress = {};
      m_TransformStart    = xiiTime::Now();
    }
    m_TransformProgress.m_bAllAssets |= bAllAssets;
    for (const auto& guid : assets)
    {
      auto* info = curator->GetAssetInfo(guid);
      if (!info || info->m_ExistanceState == xiiAssetExistanceState::FileRemoved)
        continue;
      const auto assetFlags = info->m_pDocumentTypeDescriptor->m_AssetDocumentFlags;
      if (assetFlags.IsSet(xiiAssetDocumentFlags::DisableTransform) ||
          (!flags.IsSet(xiiTransformFlags::TriggeredManually) && assetFlags.IsSet(xiiAssetDocumentFlags::OnlyTransformManually) && !finalTypes.Contains(info->m_Info->m_sAssetsDocumentTypeName)))
        continue;
      const auto owner = info->m_Info->m_DocumentID;
      if (m_RequestedAssets.Contains(owner))
      {
        const bool bFinal     = finalTypes.Contains(info->m_Info->m_sAssetsDocumentTypeName);
        bool       bCoalesced = false;
        auto&      pending    = bFinal ? m_FinalTransformRequests : m_TransformRequests;
        for (auto& queued : pending)
        {
          if (queued.m_AssetGuid == owner)
          {
            queued.m_TransformFlags.Add(flags);
            bCoalesced = true;
          }
        }
        if (!bFinal && !bCoalesced)
        {
          for (const auto& task : m_ProcessTasks)
            if (task.m_bRequested && task.m_Request.m_AssetGuid == owner &&
                (task.m_Request.m_TransformFlags.GetValue() & flags.GetValue()) == flags.GetValue())
              bCoalesced = true;
        }
        if (bCoalesced)
          continue;
        // Stronger flags cannot alter a message already sent. Queue one additional pass;
        // full-batch finalization likewise must follow ordinary in-flight transformations.
      }
      xiiProcessAssetMsg request;
      // Hashing and dependency discovery belong to the processor thread, not a GUI batch loop.
      request.m_AssetGuid      = owner;
      request.m_sAssetPath     = info->m_Path;
      request.m_sPlatform      = curator->GetActiveAssetProfile()->GetConfigName();
      request.m_TransformFlags = flags;
      if (finalTypes.Contains(info->m_Info->m_sAssetsDocumentTypeName))
      {
        request.m_TransformFlags.Add(xiiTransformFlags::ForceTransform | xiiTransformFlags::TriggeredManually);
        m_FinalTransformRequests.PushBack(std::move(request));
      }
      else
        m_TransformRequests.PushBack(std::move(request));
      ++m_RequestedAssets[owner];
      ++m_TransformProgress.m_uiTotal;
      m_TransformProgress.m_bActive = true;
    }
    BroadcastTransformProgress();
  }
  if (GetTransformProgress().m_bActive)
    StartProcessTask(false);
  return XII_SUCCESS;
}

xiiStatus xiiAssetProcessor::QueueTransformAllAssets()
{
  xiiDynamicArray<xiiUuid> assets;
  {
    const auto table = xiiAssetCurator::GetSingleton()->GetKnownAssets();
    for (auto it = table->GetIterator(); it.IsValid(); ++it)
      assets.PushBack(it.Key());
  }
  // Stable submission order; workers still reserve complete dependency hulls before work.
  assets.Sort();
  return QueueTransforms(assets, xiiTransformFlags::None, true);
}

xiiAssetTransformProgress xiiAssetProcessor::GetTransformProgress() const
{
  XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
  return m_TransformProgress;
}

void xiiAssetProcessor::BroadcastTransformProgress(bool bFinished)
{
  // Caller holds the curator mutex. All consumers copy the snapshot and queue GUI work.
  if (!m_TransformProgress.m_bActive)
    return;
  const auto duration = xiiTime::Now() - m_TransformStart;
  // Cached batches can finish many assets in one worker tick. Bound queued GUI traffic
  // while always delivering admission, cancellation and the exact terminal snapshot.
  if (!bFinished && !m_TransformProgress.m_bCancelRequested && m_TransformProgress.m_uiCompleted > 0 &&
      duration - m_TransformProgress.m_Duration < xiiTime::MakeFromMilliseconds(50))
    return;
  m_TransformProgress.m_Duration = duration;
  if (bFinished)
    m_TransformProgress.m_bActive = false;
  xiiAssetProcessorEvent event;
  event.m_Type     = bFinished ? xiiAssetProcessorEvent::Type::TransformsFinished : xiiAssetProcessorEvent::Type::TransformProgress;
  event.m_Progress = m_TransformProgress;
  m_Events.Broadcast(event);
}

xiiResult xiiAssetProcessor::PrepareAssetRequest(xiiUuid assetGuid, xiiProcessAssetMsg& out_request, xiiStringView sProfile)
{
  auto* curator = xiiAssetCurator::GetSingleton();
  XII_LOCK(curator->m_CuratorMutex);
  const auto* asset = curator->GetAssetInfo(assetGuid);
  if (asset == nullptr || asset->m_ExistanceState == xiiAssetExistanceState::FileRemoved)
    return XII_FAILURE;
  const xiiPlatformProfile* profile = curator->GetActiveAssetProfile();
  if (!sProfile.IsEmpty())
  {
    const xiiString name(sProfile);
    const auto      index = curator->FindAssetProfileByName(name);
    if (index == xiiInvalidIndex)
      return XII_FAILURE;
    profile = curator->GetAssetProfile(index);
  }

  xiiProcessAssetMsg request;
  request.m_AssetGuid  = asset->m_Info->m_DocumentID;
  request.m_sAssetPath = asset->m_Path;
  request.m_sPlatform  = profile->GetConfigName();
  curator->IsAssetUpToDate(request.m_AssetGuid, profile, asset->m_pDocumentTypeDescriptor, request.m_AssetHash, request.m_ThumbHash, request.m_PackageHash);

  xiiSet<xiiString> dependencies;
  xiiStringBuilder  root;
  curator->GenerateTransitiveHull(xiiConversionUtils::ToString(request.m_AssetGuid, root), dependencies, true, true, true);
  request.m_DepRefHull.Reserve(dependencies.GetCount());
  for (const auto& dependency : dependencies)
    request.m_DepRefHull.PushBack(dependency);

  out_request = std::move(request);
  return XII_SUCCESS;
}
