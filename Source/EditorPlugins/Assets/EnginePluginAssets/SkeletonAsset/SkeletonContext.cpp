/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>

#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <SharedPluginAssets/SkeletonAsset/SkeletonPreviewMessages.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkeletonContext, 1, xiiRTTIDefaultAllocator<xiiSkeletonContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Skeleton"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSkeletonContext::xiiSkeletonContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiSkeletonContext::HandleMessage(const xiiEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = xiiDynamicCast<const xiiEditorEngineSkeletonPreviewMsg*>(pDocMsg))
  {
    xiiUniquePtr<xiiEditableSkeleton> authoring;
    if (pMsg->GetSkeleton(authoring).Failed())
      xiiLog::Warning("Rejected invalid skeleton preview metadata.");
    else
    {
      m_pAuthoring    = std::move(authoring);
      m_bPreviewDirty = true;
    }
    return;
  }
  if (auto* pMsg = xiiDynamicCast<const xiiQuerySelectionBBoxMsgToEngine*>(pDocMsg))
  {
    QuerySelectionBBox(pMsg);
    return;
  }
  if (auto* pMsg = xiiDynamicCast<const xiiSimpleDocumentConfigMsgToEngine*>(pDocMsg))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState" && pMsg->m_sPayload == "Grid")
    {
      m_bDisplayGrid = pMsg->m_PayloadValue.ConvertTo<float>() > 0;
      return;
    }
    if (pMsg->m_sWhatToDo == "HighlightBones")
    {
      if (m_Settings.m_sHighlightedBones != pMsg->m_sPayload)
      {
        m_Settings.m_sHighlightedBones = pMsg->m_sPayload;
        m_bPreviewDirty                = true;
      }
      return;
    }
    bool* pSetting = nullptr;
    if (pMsg->m_sWhatToDo == "RenderBones")
      pSetting = &m_Settings.m_bBones;
    else if (pMsg->m_sWhatToDo == "RenderColliders")
      pSetting = &m_Settings.m_bColliders;
    else if (pMsg->m_sWhatToDo == "RenderJoints")
      pSetting = &m_Settings.m_bJoints;
    else if (pMsg->m_sWhatToDo == "RenderSwingLimits")
      pSetting = &m_Settings.m_bSwingLimits;
    else if (pMsg->m_sWhatToDo == "RenderTwistLimits")
      pSetting = &m_Settings.m_bTwistLimits;
    if (pSetting != nullptr)
    {
      const bool value = pMsg->m_PayloadValue.ConvertTo<bool>();
      m_bPreviewDirty |= *pSetting != value;
      *pSetting = value;
      return;
    }
    if (pMsg->m_sWhatToDo == "PreviewMesh")
    {
      if (m_sAnimatedMeshToUse != pMsg->m_sPayload)
      {
        XII_LOCK(m_pWorld->GetWriteMarker());
        m_sAnimatedMeshToUse           = pMsg->m_sPayload;
        xiiSkinnedMeshComponent* pMesh = nullptr;
        if (m_pWorld->TryGetComponent(m_hAnimMeshComponent, pMesh))
          pMesh->DeleteComponent();
        m_hAnimMeshComponent.Invalidate();
        m_uiMeshChangeCounter = xiiInvalidIndex;
        if (!m_sAnimatedMeshToUse.IsEmpty())
        {
          m_hAnimMeshComponent = xiiSkinnedMeshComponent::CreateComponent(m_pGameObject, pMesh);
          pMesh->SetMesh(xiiResourceManager::LoadResource<xiiMeshResource>(m_sAnimatedMeshToUse));
        }
      }
      return;
    }
  }
  xiiEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void xiiSkeletonContext::OnInitialize()
{
  XII_LOCK(m_pWorld->GetWriteMarker());
  xiiGameObjectDescription object;
  object.m_sName.Assign("SkeletonPreview");
  object.m_bDynamic = true;
  m_pWorld->CreateObject(object, m_pGameObject);
  xiiStringBuilder guid;
  xiiConversionUtils::ToString(GetDocumentGuid(), guid);
  m_hSkeleton = xiiResourceManager::LoadResource<xiiSkeletonResource>(guid);
}

void xiiSkeletonContext::OnDeinitialize()
{
  m_pGameObject = nullptr;
  m_hSkeleton.Invalidate();
  m_hAnimMeshComponent.Invalidate();
  m_sAnimatedMeshToUse.Clear();
  m_pAuthoring.Clear();
  m_Settings = {};
  m_Preview.Clear();
  m_AnimationPreview        = {};
  m_uiSkeletonChangeCounter = xiiInvalidIndex;
  m_uiMeshChangeCounter     = xiiInvalidIndex;
  m_bPreviewDirty           = true;
  m_bMeshPoseDirty          = true;
  m_bDisplayGrid            = true;
}

xiiResult xiiSkeletonContext::UpdatePreview()
{
  bool success = false;
  XII_SCOPE_EXIT(if (!success) {
    xiiSkinnedMeshComponent* pMesh = nullptr;
    if (m_pWorld->TryGetComponent(m_hAnimMeshComponent, pMesh))
      pMesh->ClearPose();
    m_bMeshPoseDirty = true;
  });
  xiiResourceLock<xiiSkeletonResource> skeleton(m_hSkeleton, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
  if (!skeleton || skeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
  {
    m_Preview.Clear();
    m_bPreviewDirty = true;
    return XII_FAILURE;
  }
  if (m_bPreviewDirty || m_uiSkeletonChangeCounter != skeleton->GetCurrentResourceChangeCounter())
  {
    m_bMeshPoseDirty |= m_uiSkeletonChangeCounter != skeleton->GetCurrentResourceChangeCounter();
    XII_SUCCEED_OR_RETURN(m_Preview.Build(skeleton->GetJoints(), m_Settings, m_pAuthoring.Borrow()));
    m_uiSkeletonChangeCounter = skeleton->GetCurrentResourceChangeCounter();
    m_bPreviewDirty           = false;
  }
  xiiSkinnedMeshComponent* pMesh = nullptr;
  if (m_pWorld->TryGetComponent(m_hAnimMeshComponent, pMesh))
  {
    xiiResourceLock<xiiMeshResource> mesh(pMesh->GetMesh(), xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!mesh || mesh.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return XII_FAILURE;
    if (m_uiMeshChangeCounter != mesh->GetCurrentResourceChangeCounter())
    {
      pMesh->ClearMaterialOverrides();
      const auto material = xiiResourceManager::LoadResource<xiiMaterialResource>("Editor/Materials/SkeletonPreviewMesh.xiiMaterial");
      for (xiiUInt32 i = 0; i < mesh->GetDescriptor().GetMaterials().GetCount(); ++i)
        pMesh->SetMaterialOverride(i, material);
      m_uiMeshChangeCounter = mesh->GetCurrentResourceChangeCounter();
      m_bMeshPoseDirty      = true;
    }
    if (m_bMeshPoseDirty)
    {
      XII_SUCCEED_OR_RETURN(m_AnimationPreview.UpdateMesh(*pMesh, {}, 0.0, m_hSkeleton));
      m_bMeshPoseDirty = false;
    }
  }
  success = true;
  return XII_SUCCESS;
}

xiiResult xiiSkeletonContext::DrawSkeleton(const xiiView& view)
{
  if (view.GetWorld() != m_pWorld)
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(UpdatePreview());
  return xiiDebugRenderer::DrawGeometry(view, m_Preview.GetGeometry(), xiiColor::White);
}

xiiBoundingBoxSphere xiiSkeletonContext::GetPreviewBounds()
{
  auto bounds = m_Preview.GetBounds();
  if (m_pGameObject != nullptr)
  {
    m_pGameObject->UpdateLocalBounds();
    m_pGameObject->UpdateGlobalTransformAndBounds();
    const auto meshBounds = m_pGameObject->GetGlobalBounds();
    if (meshBounds.IsValid())
      bounds.ExpandToInclude(meshBounds);
  }
  return bounds;
}

xiiEngineProcessViewContext* xiiSkeletonContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiSkeletonViewContext, this);
}

void xiiSkeletonContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

bool xiiSkeletonContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  XII_LOCK(m_pWorld->GetWriteMarker());
  if (UpdatePreview().Failed())
    return false;
  const auto bounds = GetPreviewBounds();

  xiiSkeletonViewContext* pMeshViewContext = static_cast<xiiSkeletonViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}

void xiiSkeletonContext::QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  XII_LOCK(m_pWorld->GetWriteMarker());
  UpdatePreview().IgnoreResult(); // A loading resource will be retried by the next frame / bounds request.
  const auto bounds = GetPreviewBounds();

  const xiiQuerySelectionBBoxMsgToEngine* msg = static_cast<const xiiQuerySelectionBBoxMsgToEngine*>(pMsg);

  xiiQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID     = msg->m_uiViewID;
  res.m_iPurpose     = msg->m_iPurpose;
  res.m_vCenter      = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
