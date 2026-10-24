/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

xiiAnimationClipViewContext::xiiAnimationClipViewContext(xiiAnimationClipContext* pContext) :
  xiiEngineProcessViewContext(pContext)
{
  m_pContext = pContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::MakeZero(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiAnimationClipViewContext::~xiiAnimationClipViewContext() = default;

bool xiiAnimationClipViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}

xiiViewHandle xiiAnimationClipViewContext::CreateView()
{
  xiiRenderPipelineResourceHandle pipeline;
  if (ResolveRenderPipeline(xiiEditorRenderPipelineUsage::ScenePreview, pipeline).Failed())
    return {};
  xiiView* pView = nullptr;
  GetDocumentContext()->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Animation Clip Editor - View", pView);
  pView->SetRenderPipelineResource(pipeline);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiAnimationClipViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  xiiEngineProcessViewContext::SetCamera(pMsg);
  if (DrawGrid(m_pContext->m_bDisplayGrid ? 0.25f : 0.0f).Failed())
    xiiLog::Warning("Could not build the animation preview grid.");
}
