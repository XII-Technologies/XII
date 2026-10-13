/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <Foundation/Utilities/ConversionUtils.h>

xiiResult xiiAssetProcessor::PrepareAssetRequest(xiiUuid assetGuid, xiiProcessAssetMsg& out_request)
{
  auto* curator = xiiAssetCurator::GetSingleton();
  XII_LOCK(curator->m_CuratorMutex);
  const auto* asset = curator->GetAssetInfo(assetGuid);
  if (asset == nullptr || asset->m_ExistanceState == xiiAssetExistanceState::FileRemoved)
    return XII_FAILURE;

  xiiProcessAssetMsg request;
  request.m_AssetGuid  = asset->m_Info->m_DocumentID;
  request.m_sAssetPath = asset->m_Path;
  request.m_sPlatform  = curator->GetActiveAssetProfile()->GetConfigName();
  curator->IsAssetUpToDate(request.m_AssetGuid, curator->GetActiveAssetProfile(), asset->m_pDocumentTypeDescriptor, request.m_AssetHash, request.m_ThumbHash, request.m_PackageHash);

  xiiSet<xiiString> dependencies;
  xiiStringBuilder  root;
  curator->GenerateTransitiveHull(xiiConversionUtils::ToString(request.m_AssetGuid, root), dependencies, true, true, true);
  request.m_DepRefHull.Reserve(dependencies.GetCount());
  for (const auto& dependency : dependencies)
    request.m_DepRefHull.PushBack(dependency);

  out_request = std::move(request);
  return XII_SUCCESS;
}
