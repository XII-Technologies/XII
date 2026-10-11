/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>

#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipContext, 1, xiiRTTIDefaultAllocator<xiiAnimationClipContext>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_CONSTANT_PROPERTY("DocumentType", (const char*) "Animation Clip"),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimationClipContext::xiiAnimationClipContext() :
  xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld)
{
}

void xiiAnimationClipContext::HandleMessage(const xiiEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = xiiDynamicCast<const xiiQuerySelectionBBoxMsgToEngine*>(pMsg0))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = xiiDynamicCast<const xiiSimpleDocumentConfigMsgToEngine*>(pMsg0))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_PayloadValue.ConvertTo<float>() > 0;
      }
    }
    else if (pMsg->m_sWhatToDo == "PreviewMesh" && m_sAnimatedMeshToUse != pMsg->m_sPayload)
    {
      m_sAnimatedMeshToUse = pMsg->m_sPayload;

      auto pWorld = m_pWorld;
      XII_LOCK(pWorld->GetWriteMarker());

      xiiSkinnedMeshComponent* mesh = nullptr;
      if (pWorld->TryGetComponent(m_hAnimMeshComponent, mesh))
      {
        mesh->ClearPose();
        mesh->SetSkeleton({});
        mesh->SetMesh(m_sAnimatedMeshToUse.IsEmpty() ? xiiMeshResourceHandle() :
                                                       xiiResourceManager::LoadResource<xiiMeshResource>(m_sAnimatedMeshToUse));
      }
    }
    else if (pMsg->m_sWhatToDo == "PlaybackPos")
    {
      SetPlaybackPosition(pMsg->m_PayloadValue.Get<double>());
    }

    return;
  }

  if (auto pMsg = xiiDynamicCast<const xiiViewRedrawMsgToEngine*>(pMsg0))
  {
    UpdatePreviewPose().IgnoreResult(); // Also refreshes resources after reload.
    xiiResourceLock<xiiAnimationClipResource> clip(m_hAnimationClip, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (clip && clip.GetAcquireResult() == xiiResourceAcquireResult::Final)
    {
      xiiSimpleDocumentConfigMsgToEditor msg;
      msg.m_DocumentGuid = pMsg->m_DocumentGuid;
      msg.m_sWhatToDo    = "ClipDuration";
      msg.m_PayloadValue = clip->GetDuration();
      SendProcessMessage(&msg);
    }
  }

  xiiEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void xiiAnimationClipContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiGameObjectDescription obj;

  // Preview
  {
    obj.m_bDynamic = true;
    obj.m_sName.Assign("SkeletonPreview");
    pWorld->CreateObject(obj, m_pGameObject);
    xiiSkinnedMeshComponent* mesh = nullptr;
    m_hAnimMeshComponent          = xiiSkinnedMeshComponent::CreateComponent(m_pGameObject, mesh);
    xiiStringBuilder clipGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), clipGuid);
    m_hAnimationClip = xiiResourceManager::LoadResource<xiiAnimationClipResource>(clipGuid);
  }
}

xiiEngineProcessViewContext* xiiAnimationClipContext::CreateViewContext()
{
  return XII_DEFAULT_NEW(xiiAnimationClipViewContext, this);
}

void xiiAnimationClipContext::DestroyViewContext(xiiEngineProcessViewContext* pContext)
{
  XII_DEFAULT_DELETE(pContext);
}

bool xiiAnimationClipContext::UpdateThumbnailViewContext(xiiEngineProcessViewContext* pThumbnailViewContext)
{
  const double previousPosition = m_fPlaybackPosition;
  XII_SCOPE_EXIT(m_fPlaybackPosition = previousPosition);
  m_fPlaybackPosition = 0.5;
  if (UpdatePreviewPose().Failed())
    return false;
  xiiBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  xiiAnimationClipViewContext* pMeshViewContext = static_cast<xiiAnimationClipViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}

void xiiAnimationClipContext::QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  xiiBoundingBoxSphere bounds = xiiBoundingBoxSphere::MakeInvalid();

  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    m_pGameObject->UpdateLocalBounds();
    m_pGameObject->UpdateGlobalTransformAndBounds();
    const auto& b = m_pGameObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const xiiQuerySelectionBBoxMsgToEngine* msg = static_cast<const xiiQuerySelectionBBoxMsgToEngine*>(pMsg);

  xiiQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID     = msg->m_uiViewID;
  res.m_iPurpose     = msg->m_iPurpose;
  res.m_vCenter      = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtents;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void xiiAnimationClipContext::SetPlaybackPosition(double pos)
{
  if (xiiMath::IsFinite(pos))
    m_fPlaybackPosition = xiiMath::Clamp(pos, 0.0, 1.0);
}

xiiResult xiiAnimationClipContext::UpdatePreviewPose()
{
  XII_LOCK(m_pWorld->GetWriteMarker());
  xiiSkinnedMeshComponent* mesh = nullptr;
  if (!m_pWorld->TryGetComponent(m_hAnimMeshComponent, mesh))
    return XII_FAILURE;
  return m_Preview.UpdateMesh(*mesh, m_hAnimationClip, m_fPlaybackPosition);
}
