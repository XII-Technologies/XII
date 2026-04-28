/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>

#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

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

      xiiStringBuilder sAnimClipGuid;
      xiiConversionUtils::ToString(GetDocumentGuid(), sAnimClipGuid);

      xiiAnimatedMeshComponent* pAnimMesh;
      if (pWorld->TryGetComponent(m_hAnimMeshComponent, pAnimMesh))
      {
        pAnimMesh->DeleteComponent();
        m_hAnimMeshComponent.Invalidate();
      }

      xiiSimpleAnimationComponent* pAnimController;
      if (pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
      {
        pAnimController->DeleteComponent();
        m_hAnimControllerComponent.Invalidate();
      }

      if (!m_sAnimatedMeshToUse.IsEmpty())
      {
        m_hAnimMeshComponent       = xiiAnimatedMeshComponent::CreateComponent(m_pGameObject, pAnimMesh);
        m_hAnimControllerComponent = xiiSimpleAnimationComponent::CreateComponent(m_pGameObject, pAnimController);

        pAnimMesh->SetMeshFile(m_sAnimatedMeshToUse);
        pAnimController->SetAnimationClipFile(sAnimClipGuid);
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
    auto pWorld = m_pWorld;
    XII_LOCK(pWorld->GetWriteMarker());

    xiiSimpleAnimationComponent* pAnimController;
    if (pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
    {
      if (pAnimController->m_hAnimationClip.IsValid())
      {
        xiiResourceLock<xiiAnimationClipResource> pResource(pAnimController->m_hAnimationClip, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);

        if (pResource.GetAcquireResult() == xiiResourceAcquireResult::Final)
        {
          xiiSimpleDocumentConfigMsgToEditor msg;
          msg.m_DocumentGuid = pMsg->m_DocumentGuid;
          msg.m_sWhatToDo    = "ClipDuration";
          msg.m_PayloadValue = pResource->GetDescriptor().GetDuration();

          SendProcessMessage(&msg);
        }
      }
    }
  }

  xiiEngineProcessDocumentContext::HandleMessage(pMsg0);
}

void xiiAnimationClipContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiGameObjectDesc obj;

  // Preview
  {
    obj.m_bDynamic = true;
    obj.m_sName.Assign("SkeletonPreview");
    pWorld->CreateObject(obj, m_pGameObject);
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
  xiiBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  if (!m_hAnimControllerComponent.IsInvalidated())
  {
    XII_LOCK(m_pWorld->GetWriteMarker());

    xiiSimpleAnimationComponent* pAnimController;
    if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
    {
      pAnimController->SetNormalizedPlaybackPosition(0.5f);
      pAnimController->m_fSpeed = 0.0f;

      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->Update();
      m_pWorld->SetWorldSimulationEnabled(false);
    }
  }

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
  XII_LOCK(m_pWorld->GetWriteMarker());

  xiiSimpleAnimationComponent* pAnimController;
  if (m_pWorld->TryGetComponent(m_hAnimControllerComponent, pAnimController))
  {
    pAnimController->SetNormalizedPlaybackPosition(static_cast<float>(pos));
    pAnimController->m_fSpeed = 0.0f;
  }
}
