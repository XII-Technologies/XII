/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graph/GraphAlgorithms.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/Progress.h>

xiiStatus xiiAssetCurator::GetAssetResaveOrder(xiiStringView sPrefixPath, xiiDynamicArray<xiiUuid>& out_order) const
{
  out_order.Clear();
  struct Asset
  {
    xiiUuid   m_Id;
    xiiString m_sPath;
  };
  xiiDynamicArray<Asset> assets;
  xiiGraph               graph;
  {
    XII_LOCK(m_CuratorMutex);
    for (auto it = m_KnownAssets.GetIterator(); it.IsValid(); ++it)
    {
      if (it.Value()->m_ExistanceState != xiiAssetExistanceState::FileRemoved &&
          (sPrefixPath.IsEmpty() || xiiPathUtils::IsSubPath(sPrefixPath, it.Value()->m_Path.GetAbsolutePath())))
        assets.PushBack({it.Key(), it.Value()->m_Path.GetAbsolutePath()});
    }
    assets.Sort([](const Asset& a, const Asset& b) {
      const auto compare = a.m_sPath.Compare(b.m_sPath);
      return compare == 0 ? a.m_Id < b.m_Id : compare < 0;
    });
    XII_SUCCEED_OR_RETURN(graph.Reserve(assets.GetCount(), 0));
    xiiHashTable<xiiUuid, xiiGraphVertexHandle> vertices;
    vertices.Reserve(assets.GetCount());
    for (const auto& asset : assets)
    {
      xiiGraphVertexHandle vertex;
      XII_SUCCEED_OR_RETURN(graph.TryAddVertex(vertex));
      vertices.Insert(asset.m_Id, vertex);
    }
    for (const auto& asset : assets)
    {
      const auto* info = GetAssetInfo(asset.m_Id)->m_Info.Borrow();
      for (const auto* dependencies : {&info->m_TransformDependencies, &info->m_ThumbnailDependencies, &info->m_PackageDependencies})
      {
        for (const auto& reference : *dependencies)
        {
          // FindSubAsset accepts UUIDs and paths and resolves sub-assets to their owning document.
          const auto dependency = FindSubAsset(reference);
          if (!dependency)
            continue; // External files and unavailable assets are not resave targets.
          xiiGraphVertexHandle source;
          if (!vertices.TryGetValue(dependency->m_pAssetInfo->m_Info->m_DocumentID, source))
            continue;
          xiiGraphEdgeHandle edge;
          XII_SUCCEED_OR_RETURN(graph.TryAddEdge(source, vertices[asset.m_Id], edge));
        }
      }
    }
  }

  xiiDynamicArray<xiiGraphVertexHandle> order;
  xiiGraphAlgorithmWorkspace            workspace;
  if (xiiGraphAlgorithms::TopologicalSort(graph, order, &workspace).Failed())
  {
    xiiGraphComponents components;
    xiiGraphAlgorithms::StronglyConnectedComponents(graph, components, &workspace);
    xiiStringBuilder message("Cannot resave assets with cyclic dependencies:");
    xiiUInt32        cyclicCount = 0;
    for (xiiUInt32 i = 0; i < components.GetCount(); ++i)
    {
      const auto component = components.GetComponent(i);
      if (component.GetCount() == 1 && graph.FindEdge(component[0], component[0]).IsInvalidated())
        continue;
      for (const auto vertex : component)
      {
        if (cyclicCount++ < 8)
          message.AppendFormat("\n{}", assets[graph.GetVertexIndex(vertex)].m_sPath);
      }
    }
    if (cyclicCount > 8)
      message.AppendFormat("\n...and {} more cyclic assets.", cyclicCount - 8);
    return xiiStatus(message.GetView());
  }
  out_order.Reserve(order.GetCount());
  for (const auto vertex : order)
    out_order.PushBack(assets[graph.GetVertexIndex(vertex)].m_Id);
  return XII_SUCCESS;
}

xiiStatus xiiAssetCurator::ResaveAllAssets(xiiStringView sPrefixPath)
{
  if (!xiiThreadUtils::IsMainThread())
    return xiiStatus("Assets must be resaved on the main thread.");
  if (m_bResavingAssets)
    return xiiStatus("An asset resave is already in progress.");
  m_bResavingAssets = true;
  XII_SCOPE_EXIT(m_bResavingAssets = false);

  xiiDynamicArray<xiiUuid> order;
  XII_SUCCEED_OR_RETURN(GetAssetResaveOrder(sPrefixPath, order));
  if (order.IsEmpty())
    return XII_SUCCESS;
  xiiProgressRange range("Re-saving Assets", order.GetCount(), true);
  xiiUInt32        failed = 0;
  for (const auto id : order)
  {
    if (range.WasCanceled())
      return xiiStatus("Asset resave was canceled.");
    xiiString path;
    {
      XII_LOCK(m_CuratorMutex);
      if (const auto* asset = GetAssetInfo(id))
        path = asset->m_Path.GetAbsolutePath();
    }
    // Progress events may pump Qt or mutate the curator. Never retain an asset-info pointer across
    // this boundary, and recheck cancellation before saving the current document.
    if (!range.BeginNextStep(xiiPathUtils::GetFileNameAndExtension(path)))
      return xiiStatus("Asset resave was canceled.");
    const auto result = ResaveAsset(id);
    if (result.Failed())
    {
      ++failed;
      xiiLog::Error("{} ({})", result.GetMessageString(), path);
    }
  }
  if (failed != 0)
    return xiiStatus(xiiFmt("Failed to resave {} of {} assets. See the log for details.", failed, order.GetCount()));
  return XII_SUCCESS;
}

xiiStatus xiiAssetCurator::ResaveAsset(const xiiUuid& assetGuid)
{
  xiiString path;
  {
    XII_LOCK(m_CuratorMutex);
    const auto* asset = GetAssetInfo(assetGuid);
    if (asset == nullptr || asset->m_ExistanceState == xiiAssetExistanceState::FileRemoved)
      return xiiStatus("The asset no longer exists.");
    path = asset->m_Path.GetAbsolutePath();
  }
  auto*      document = xiiDocumentManager::GetDocumentByGuid(assetGuid);
  const bool wasOpen  = document != nullptr;
  if (!document)
    document = xiiQtEditorApp::GetSingleton()->OpenDocument(path, xiiDocumentFlags::None);
  if (!document)
    return xiiStatus(xiiFmt("Could not open asset document '{}'.", path));
  XII_SCOPE_EXIT(if (!wasOpen && !document->HasWindowBeenRequested()) document->GetDocumentManager()->CloseDocument(document));
  if (document->GetGuid() != assetGuid)
    return xiiStatus(xiiFmt("Asset identity changed while opening '{}'.", path));
  if (document->IsReadOnly() || document->GetUnknownObjectTypeInstances() != 0)
    return xiiStatus(xiiFmt("Cannot resave read-only or partially understood asset '{}'.", path));
  return document->SaveDocument(true);
}
