/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonContext.h>
#include <EnginePluginAssets/SkeletonAsset/SkeletonView.h>
#include <GraphicsCore/Debug/DebugRenderer.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

xiiSkeletonViewContext::xiiSkeletonViewContext(xiiSkeletonContext* pContext) :
  xiiEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::MakeZero(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiSkeletonViewContext::~xiiSkeletonViewContext() = default;

bool xiiSkeletonViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  if (const auto* pView = GetView())
    m_pContext->DrawSkeleton(*pView).IgnoreResult();
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}

xiiViewHandle xiiSkeletonViewContext::CreateView()
{
  xiiRenderPipelineResourceHandle pipeline;
  if (ResolveRenderPipeline(xiiEditorRenderPipelineUsage::ScenePreview, pipeline).Failed())
    return {};
  xiiView* pView = nullptr;
  GetDocumentContext()->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Skeleton Editor - View", pView);
  pView->SetRenderPipelineResource(pipeline);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiSkeletonViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  xiiEngineProcessViewContext::SetCamera(pMsg);
  if (const auto* pView = GetView())
    m_pContext->DrawSkeleton(*pView).IgnoreResult(); // Resources may still be loading.
  if (DrawGrid(m_pContext->m_bDisplayGrid ? 0.25f : 0.0f).Failed())
    xiiLog::Warning("Could not build the skeleton preview grid.");

  const xiiUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hSkeleton = m_pContext->GetSkeleton();
  if (hSkeleton.IsValid())
  {
    xiiResourceLock<xiiSkeletonResource> pSkeleton(hSkeleton, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!pSkeleton || pSkeleton.GetAcquireResult() != xiiResourceAcquireResult::Final)
      return;

    xiiUInt32 uiNumJoints = pSkeleton->GetJointCount();

    xiiStringBuilder sText;
    sText.AppendFormat("Joints: {}\n", uiNumJoints);

    if (const xiiView* pView = GetView())
      xiiDebugRenderer::Draw2DText(*pView, sText, xiiVec2I32(10, viewHeight - 10), xiiColor::White, 16, xiiDebugTextHAlign::Left, xiiDebugTextVAlign::Bottom);
  }
}

void xiiSkeletonViewContext::HandleViewMessage(const xiiEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<xiiViewPickingMsgToEngine>())
  {
    const xiiViewPickingMsgToEngine* pMsg2 = static_cast<const xiiViewPickingMsgToEngine*>(pMsg);

    PickObjectAt(pMsg2->m_uiPickPosX, pMsg2->m_uiPickPosY, pMsg2->m_uiRequestID, pMsg2->m_bPickGizmos);
  }
  else
  {
    xiiEngineProcessViewContext::HandleViewMessage(pMsg);
  }
}

void xiiSkeletonViewContext::PickObjectAt(xiiUInt16 x, xiiUInt16 y, xiiUInt64 uiRequestID, bool bPickGizmos)
{
  xiiViewPickingResultMsgToEditor result;
  result.m_uiRequestID = uiRequestID;
  if (bPickGizmos)
    PickGizmoAtPixel(x, y, result).IgnoreResult();
  else
  {
    result.m_uiPartIndex = 0;
    result.m_vPickedPosition.SetZero();
    result.m_vPickedNormal.SetZero();
    result.m_vPickingRayStartPosition.SetZero();
  }
  SendViewMessage(&result);
}
