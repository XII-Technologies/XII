#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>

#include <RendererCore/AnimationSystem/SkeletonComponent.h>
#include <RendererCore/AnimationSystem/SkeletonPoseComponent.h>

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
  if (auto pMsg = xiiDynamicCast<const xiiQuerySelectionBBoxMsgToEngine*>(pDocMsg))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = xiiDynamicCast<const xiiSimpleDocumentConfigMsgToEngine*>(pDocMsg))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_fPayload > 0;
        return;
      }
    }
    else if (pMsg->m_sWhatToDo == "HighlightBones")
    {
      XII_LOCK(m_pWorld->GetWriteMarker());

      xiiSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->SetBonesToHighlight(pMsg->m_sPayload);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderBones")
    {
      XII_LOCK(m_pWorld->GetWriteMarker());

      xiiSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeBones = (pMsg->m_fPayload != 0);
      }

      // resend the pose every frame (this config message is send every frame)
      // this ensures that changing any of the visualization states in the skeleton component displays correctly
      // a bit hacky and should be cleaned up, but this way the skeleton component doesn't need to keep a copy of the last pose (maybe it should)
      xiiSkeletonPoseComponent* pPoseSkeleton;
      if (m_pWorld->TryGetComponent(m_hPoseComponent, pPoseSkeleton))
      {
        pPoseSkeleton->ResendPose();
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderColliders")
    {
      XII_LOCK(m_pWorld->GetWriteMarker());

      xiiSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeColliders = (pMsg->m_fPayload != 0);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderJoints")
    {
      XII_LOCK(m_pWorld->GetWriteMarker());

      xiiSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeJoints = (pMsg->m_fPayload != 0);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderSwingLimits")
    {
      XII_LOCK(m_pWorld->GetWriteMarker());

      xiiSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeSwingLimits = (pMsg->m_fPayload != 0);
      }
    }
    else if (pMsg->m_sWhatToDo == "RenderTwistLimits")
    {
      XII_LOCK(m_pWorld->GetWriteMarker());

      xiiSkeletonComponent* pSkeleton = nullptr;
      if (m_pWorld->TryGetComponent(m_hSkeletonComponent, pSkeleton))
      {
        pSkeleton->m_bVisualizeTwistLimits = (pMsg->m_fPayload != 0);
      }
    }
  }

  xiiEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void xiiSkeletonContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  XII_LOCK(pWorld->GetWriteMarker());

  xiiGameObjectDesc         obj;
  xiiSkeletonComponent*     pVisSkeleton;
  xiiSkeletonPoseComponent* pPoseSkeleton;

  // Preview Mesh
  {
    obj.m_sName.Assign("SkeletonPreview");
    pWorld->CreateObject(obj, m_pGameObject);

    m_hSkeletonComponent = xiiSkeletonComponent::CreateComponent(m_pGameObject, pVisSkeleton);
    xiiStringBuilder sSkeletonGuid;
    xiiConversionUtils::ToString(GetDocumentGuid(), sSkeletonGuid);
    m_hSkeleton = xiiResourceManager::LoadResource<xiiSkeletonResource>(sSkeletonGuid);
    pVisSkeleton->SetSkeleton(m_hSkeleton);

    m_hPoseComponent = xiiSkeletonPoseComponent::CreateComponent(m_pGameObject, pPoseSkeleton);
    pPoseSkeleton->SetSkeleton(m_hSkeleton);
    pPoseSkeleton->SetPoseMode(xiiSkeletonPoseMode::RestPose);
  }
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
  xiiBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  xiiSkeletonViewContext* pMeshViewContext = static_cast<xiiSkeletonViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void xiiSkeletonContext::QuerySelectionBBox(const xiiEditorEngineDocumentMsg* pMsg)
{
  if (m_pGameObject == nullptr)
    return;

  xiiBoundingBoxSphere bounds;
  bounds.SetInvalid();

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
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}
