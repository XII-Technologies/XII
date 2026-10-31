/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

void xiiAssetProcessor::ApplyImportResults()
{
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "Apply imported document changes on the editor thread.");
  XII_LOCK(m_ProcessorMutex);
  XII_LOCK(xiiAssetCurator::GetSingleton()->m_CuratorMutex);
  for (auto it = m_DeferredImports.GetIterator(); it.IsValid();)
  {
    auto* document = xiiDocumentManager::GetDocumentByGuid(it.Key());
    if (!document || !document->IsModified())
      it = m_DeferredImports.Remove(it);
    else
      ++it;
  }
  for (auto& task : m_ProcessTasks)
  {
    if (!task.m_bImportPending)
      continue;
    task.m_bImportPending = false;
    task.m_bImportApplied = true;
    if (!xiiToolsProject::IsProjectGenerationCurrent(m_uiProjectGeneration) || m_ProcessTaskState != ProcessTaskState::Running || !xiiToolsProject::IsProjectOpen() || xiiToolsProject::IsProjectClosing())
    {
      task.m_Status = xiiTransformStatus(xiiTransformResult::NeedsImport);
      continue;
    }
    xiiProcessAssetMsg current;
    if (PrepareAssetRequest(task.m_AssetGuid, current, task.m_Request.m_sPlatform).Failed() ||
        current.m_AssetHash != task.m_Request.m_AssetHash || current.m_ThumbHash != task.m_Request.m_ThumbHash || current.m_PackageHash != task.m_Request.m_PackageHash)
    {
      // ReportResult schedules a fresh pass for changed saved inputs.
      continue;
    }
    auto*      document = xiiDocumentManager::GetDocumentByGuid(task.m_AssetGuid);
    const bool bWasOpen = document != nullptr;
    if (!document)
      document = xiiQtEditorApp::GetSingleton()->OpenDocument(task.m_Request.m_sAssetPath, xiiDocumentFlags::None);
    if (!document)
    {
      task.m_Status = xiiTransformStatus("Could not open asset to apply its background import.");
      continue;
    }
    XII_SCOPE_EXIT(if (!bWasOpen) document->GetDocumentManager()->CloseDocument(document));
    xiiAbstractObjectGraph   before, after, live;
    xiiRawMemoryStreamReader beforeStream(task.m_sImportBefore.GetData(), task.m_sImportBefore.GetElementCount());
    xiiRawMemoryStreamReader afterStream(task.m_sImportAfter.GetData(), task.m_sImportAfter.GetElementCount());
    if (xiiAbstractGraphDdlSerializer::Read(beforeStream, &before).Failed() || xiiAbstractGraphDdlSerializer::Read(afterStream, &after).Failed())
    {
      task.m_Status = xiiTransformStatus("Invalid document graph returned by the asset importer.");
      continue;
    }
    xiiDocumentObjectConverterWriter writer(&live, document->GetObjectManager(), [](const xiiDocumentObject*, const xiiAbstractProperty* property) {
      return property->GetAttributeByType<xiiTemporaryAttribute>() == nullptr;
    });
    auto*                            root       = document->GetObjectManager()->GetRootObject();
    const auto*                      beforeRoot = before.GetNodeByName("ObjectTree");
    const auto*                      afterRoot  = after.GetNodeByName("ObjectTree");
    if (!beforeRoot || !afterRoot || beforeRoot->GetGuid() != root->GetGuid() || afterRoot->GetGuid() != root->GetGuid())
    {
      task.m_Status = xiiTransformStatus("Asset importer returned an unexpected document root.");
      continue;
    }
    writer.AddObjectToGraph(root, "ObjectTree");
    xiiDeque<xiiAbstractGraphDiffOperation> difference;
    live.CreateDiffWithBaseGraph(before, difference);
    if (document->IsModified() || !difference.IsEmpty())
    {
      // Keep the user's edits. Saving the document makes it eligible for another
      // import; never overwrite a live document with a stale worker snapshot.
      task.m_Status = xiiTransformStatus(xiiTransformResult::NeedsImport);
      m_DeferredImports.Insert(task.m_AssetGuid);
      continue;
    }
    after.CreateDiffWithBaseGraph(before, difference);
    if (!difference.IsEmpty())
    {
      auto* accessor = document->GetObjectAccessor();
      accessor->StartTransaction("Apply Background Asset Import");
      xiiDocumentObjectConverterReader::ApplyDiffToObject(accessor, root, difference);
      accessor->FinishTransaction();
      task.m_Status = document->SaveDocument();
    }
  }
}

xiiStatus xiiAssetProcessor::QueueTransformAssets(xiiArrayPtr<const xiiUuid> assets, xiiBitflags<xiiTransformFlags> flags)
{
  return QueueTransforms(assets, flags, false);
}

xiiStatus xiiAssetProcessor::QueueTransformAssetReferences(xiiArrayPtr<const xiiString> references, xiiBitflags<xiiTransformFlags> flags)
{
  xiiDynamicArray<xiiUuid> assets;
  assets.Reserve(references.GetCount());
  for (const auto& reference : references)
  {
    xiiUuid guid;
    XII_SUCCEED_OR_RETURN(xiiAssetCurator::GetSingleton()->ResolveAsset(reference, guid));
    assets.PushBack(guid);
  }
  return QueueTransforms(assets, flags, false);
}

xiiStatus xiiAssetProcessor::QueueTransforms(xiiArrayPtr<const xiiUuid> assets, xiiBitflags<xiiTransformFlags> flags, bool bAllAssets)
{
  XII_ASSERT_DEV(xiiThreadUtils::IsMainThread(), "Save and queue transformations on the editor thread.");
  if (!xiiToolsProject::IsProjectOpen() || !xiiAssetCurator::GetSingleton()->IsReady())
    return xiiStatus("No project is available for transformation.");

  const auto                  generation = xiiToolsProject::GetProjectGeneration();
  auto*                       curator    = xiiAssetCurator::GetSingleton();
  xiiSet<xiiTempHashedString> finalTypes;
  if (bAllAssets)
  {
    for (auto* manager : xiiDocumentManager::GetAllDocumentManagers())
      if (auto* assetManager = xiiDynamicCast<xiiAssetDocumentManager*>(manager))
        assetManager->GetAssetTypesRequiringTransformForSceneExport(finalTypes);
  }
  xiiDynamicArray<xiiUuid> owners;
  {
    const auto known = curator->GetKnownAssets();
    for (const auto guid : assets)
    {
      const auto* info = curator->GetAssetInfo(guid);
      if (info == nullptr)
        return xiiStatus("A queued asset no longer exists. Refresh the selection before transforming.");
      if (!owners.Contains(info->m_Info->m_DocumentID))
        owners.PushBack(info->m_Info->m_DocumentID);
    }
  }
  assets = owners.GetArrayPtr();
  // Save documents only on the GUI thread. The subprocess reads saved inputs and never
  // modifies the editor's live document graph. Import graphs are applied by the curator tick.
  for (const auto& guid : assets)
  {
    if (auto* document = xiiDocumentManager::GetDocumentByGuid(guid); flags.IsSet(xiiTransformFlags::TriggeredManually) && document && document->IsModified())
      XII_SUCCEED_OR_RETURN(document->SaveDocument());
  }
  if (!xiiToolsProject::IsProjectGenerationCurrent(generation) || !xiiToolsProject::IsProjectOpen() || !curator->IsReady())
    return xiiStatus("The project changed while saving transformation inputs. Submit the request in the current project.");
  {
    XII_LOCK(m_ProcessorMutex);
    if (m_ProcessTaskState == ProcessTaskState::Stopping)
      return xiiStatus("Asset processing is stopping. Retry once it has stopped.");
    XII_LOCK(curator->m_CuratorMutex);
    if (!m_TransformProgress.m_bActive)
    {
      m_TransformProgress                       = {};
      m_TransformProgress.m_uiProjectGeneration = xiiToolsProject::GetProjectGeneration();
      m_TransformStart                          = xiiTime::Now();
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
      if (flags.IsSet(xiiTransformFlags::TriggeredManually))
        m_DeferredImports.Remove(owner);
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
  if (m_TransformProgress.m_uiProjectGeneration != xiiToolsProject::GetProjectGeneration())
    return {};
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
  if (!curator->IsReady() || !xiiToolsProject::IsProjectOpen())
    return XII_FAILURE;
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
