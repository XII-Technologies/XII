#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/AnimationClipAsset/AnimationClipContext.h>
#include <EnginePluginAssets/AnimationClipAsset/AnimationClipView.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

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
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Animation Clip Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}

void xiiAnimationClipViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  if (m_pContext->m_bDisplayGrid)
  {
    xiiEngineProcessViewContext::DrawSimpleGrid();
  }

  xiiEngineProcessViewContext::SetCamera(pMsg);
}
