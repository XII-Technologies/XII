/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <Foundation/Utilities/DGMLWriter.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

void xiiAssetCurator::FindAllUses(xiiUuid assetGuid, xiiSet<xiiUuid>& ref_uses, bool bTransitive) const
{
  XII_LOCK(m_CuratorMutex);

  xiiSet<xiiUuid> todoList;
  todoList.Insert(assetGuid);
  xiiSet<xiiUuid> visited;

  auto GatherReferences = [&](const xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>>& inverseTracker, const xiiStringBuilder& sAsset) {
    auto it = inverseTracker.Find(sAsset);
    if (it.IsValid())
    {
      for (const xiiUuid& guid : it.Value())
      {
        if (!visited.Contains(guid))
          todoList.Insert(guid);

        ref_uses.Insert(guid);
      }
    }
  };

  xiiStringBuilder sCurrentAsset;
  do
  {
    auto                itFirst = todoList.GetIterator();
    const xiiAssetInfo* pInfo   = GetAssetInfo(itFirst.Key());
    visited.Insert(itFirst.Key());
    todoList.Remove(itFirst);

    if (pInfo)
    {
      sCurrentAsset = pInfo->m_Path;
      GatherReferences(m_InverseThumbnailDeps, sCurrentAsset);
      GatherReferences(m_InverseTransformDeps, sCurrentAsset);
      GatherReferences(m_InversePackageDeps, sCurrentAsset);
    }
  } while (bTransitive && !todoList.IsEmpty());
}

void xiiAssetCurator::FindAllUses(xiiStringView sPathOrGuid, xiiSet<xiiUuid>& ref_uses) const
{
  XII_LOCK(m_CuratorMutex);
  if (const auto asset = FindSubAsset(sPathOrGuid))
  {
    FindAllUses(asset->m_Data.m_Guid, ref_uses, false);
    return;
  }
  xiiDataDirPath path;
  if (xiiFileSystemModel::GetSingleton()->ResolveFilePath(sPathOrGuid, path).Failed())
    return;
  for (const auto* tracker : {&m_InverseTransformDeps, &m_InverseThumbnailDeps, &m_InversePackageDeps})
    if (const auto it = tracker->Find(path.GetAbsolutePath()); it.IsValid())
      for (const auto id : it.Value())
        ref_uses.Insert(id);
}

bool xiiAssetCurator::IsReferenced(xiiStringView sPathOrGuid) const
{
  xiiSet<xiiUuid> uses;
  FindAllUses(sPathOrGuid, uses);
  return !uses.IsEmpty();
}

void xiiAssetCurator::GenerateTransitiveHull(const xiiStringView sAssetOrPath, xiiSet<xiiString>& inout_deps, bool bIncludeTransformDeps, bool bIncludeThumbnailDeps, bool bIncludePackageDeps) const
{
  XII_LOCK(m_CuratorMutex);

  xiiHybridArray<xiiString, 6> toDoList;
  xiiSet<xiiUuid>              visited;
  inout_deps.Insert(sAssetOrPath);
  toDoList.PushBack(sAssetOrPath);

  while (!toDoList.IsEmpty())
  {
    xiiString currentAsset = toDoList.PeekBack();
    toDoList.PopBack();

    const auto asset = FindSubAsset(currentAsset);
    if (asset && !visited.Contains(asset->m_pAssetInfo->m_Info->m_DocumentID))
    {
      const xiiAssetInfo* pAssetInfo = asset->m_pAssetInfo;
      visited.Insert(pAssetInfo->m_Info->m_DocumentID);

      auto GatherDependencies = [&](const xiiSet<xiiString>& dependencies) {
        for (const xiiString& dep : dependencies)
        {
          inout_deps.Insert(dep);
          const auto dependency = FindSubAsset(dep);
          if (dependency && !visited.Contains(dependency->m_pAssetInfo->m_Info->m_DocumentID))
          {
            toDoList.PushBack(dep);
          }
        }
      };
      if (bIncludeTransformDeps)
        GatherDependencies(pAssetInfo->m_Info->m_TransformDependencies);
      if (bIncludeThumbnailDeps)
        GatherDependencies(pAssetInfo->m_Info->m_ThumbnailDependencies);
      if (bIncludePackageDeps)
        GatherDependencies(pAssetInfo->m_Info->m_PackageDependencies);
    }
  }
}

void xiiAssetCurator::GenerateInverseTransitiveHull(const xiiAssetInfo* pAssetInfo, xiiSet<xiiUuid>& inout_inverseDeps, bool bIncludeTransformDeps, bool bIncludeThumbnailDeps, bool bIncludePackageDeps) const
{
  XII_LOCK(m_CuratorMutex);

  xiiHybridArray<const xiiAssetInfo*, 6> toDoList;
  toDoList.PushBack(pAssetInfo);
  inout_inverseDeps.Insert(pAssetInfo->m_Info->m_DocumentID);

  while (!toDoList.IsEmpty())
  {
    const xiiAssetInfo* currentAsset = toDoList.PeekBack();
    toDoList.PopBack();

    auto GatherDependencies = [&](const xiiMap<xiiString, xiiHybridArray<xiiUuid, 1>>& tracker) {
      if (auto it = tracker.Find(currentAsset->m_Path.GetAbsolutePath()); it.IsValid())
      {
        for (const xiiUuid& asset : it.Value())
        {
          if (!inout_inverseDeps.Contains(asset))
          {
            xiiAssetInfo* pAssetInfo = nullptr;
            if (m_KnownAssets.TryGetValue(asset, pAssetInfo))
            {
              toDoList.PushBack(pAssetInfo);
              inout_inverseDeps.Insert(asset);
            }
          }
        }
      }
    };
    if (bIncludeTransformDeps)
      GatherDependencies(m_InverseTransformDeps);
    if (bIncludeThumbnailDeps)
      GatherDependencies(m_InverseThumbnailDeps);
    if (bIncludePackageDeps)
      GatherDependencies(m_InversePackageDeps);
  }
}

xiiResult xiiAssetCurator::WriteDependencyDGML(const xiiUuid& guid, xiiStringView sOutputFile) const
{
  XII_LOCK(m_CuratorMutex);

  xiiDGMLGraph graph;

  xiiSet<xiiString> deps;
  xiiStringBuilder  sTemp;
  GenerateTransitiveHull(xiiConversionUtils::ToString(guid, sTemp), deps, true, true, true);

  xiiHashTable<xiiString, xiiUInt32> nodeMap;
  nodeMap.Reserve(deps.GetCount());
  for (auto& dep : deps)
  {
    xiiDGMLGraph::NodeDesc nd;
    const auto             asset = FindSubAsset(dep);
    if (asset)
    {
      const xiiSubAsset&  subAsset   = *asset;
      const xiiAssetInfo* pAssetInfo = subAsset.m_pAssetInfo;
      if (subAsset.m_bMainAsset)
      {
        nd.m_Color = xiiColor::Blue;
        sTemp.SetFormat("{}", pAssetInfo->m_Path.GetDataDirParentRelativePath());
      }
      else
      {
        nd.m_Color = xiiColor::AliceBlue;
        sTemp.SetFormat("{} | {}", pAssetInfo->m_Path.GetDataDirParentRelativePath(), subAsset.GetName());
      }
      nd.m_Shape = xiiDGMLGraph::NodeShape::Rectangle;
    }
    else
    {
      sTemp      = dep;
      nd.m_Color = xiiConversionUtils::IsStringUuid(dep) ? xiiColor::Red : xiiColor::Orange;
      nd.m_Shape = xiiDGMLGraph::NodeShape::Rectangle;
    }
    xiiUInt32 uiGraphNode = graph.AddNode(sTemp, &nd);
    nodeMap.Insert(dep, uiGraphNode);
  }

  for (auto& node : deps)
  {
    const auto asset = FindSubAsset(node);
    if (asset)
    {
      xiiUInt32 uiInputNode = *nodeMap.GetValue(node);

      const xiiAssetInfo* pAssetInfo = asset->m_pAssetInfo;

      xiiMap<xiiUInt32, xiiString> connection;

      auto ExtendConnection = [&](const xiiString& sRef, xiiStringView sLabel) {
        xiiUInt32 uiOutputNode = *nodeMap.GetValue(sRef);
        sTemp                  = connection[uiOutputNode];
        if (sTemp.IsEmpty())
          sTemp = sLabel;
        else
          sTemp.AppendFormat(" | {}", sLabel);
        connection[uiOutputNode] = sTemp;
      };

      for (const xiiString& sRef : pAssetInfo->m_Info->m_TransformDependencies)
      {
        ExtendConnection(sRef, "Transform");
      }

      for (const xiiString& sRef : pAssetInfo->m_Info->m_ThumbnailDependencies)
      {
        ExtendConnection(sRef, "Thumbnail");
      }

      for (const xiiString& ref : pAssetInfo->m_Info->m_PackageDependencies)
      {
        ExtendConnection(ref, "Package");
      }

      for (auto it : connection)
      {
        graph.AddConnection(uiInputNode, it.Key(), it.Value());
      }
    }
  }

  return xiiDGMLGraphWriter::WriteGraphToFile(sOutputFile, graph);
}
