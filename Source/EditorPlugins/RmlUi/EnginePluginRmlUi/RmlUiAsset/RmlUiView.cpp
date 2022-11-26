#include <EnginePluginRmlUi/EnginePluginRmlUiPCH.h>

#include <EnginePluginRmlUi/RmlUiAsset/RmlUiContext.h>
#include <EnginePluginRmlUi/RmlUiAsset/RmlUiView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiRmlUiViewContext::xiiRmlUiViewContext(xiiRmlUiDocumentContext* pRmlUiContext) :
  xiiEngineProcessViewContext(pRmlUiContext)
{
  m_pRmlUiContext = pRmlUiContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.01f, 1000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::ZeroVector(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiRmlUiViewContext::~xiiRmlUiViewContext() {}

bool xiiRmlUiViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}

xiiViewHandle xiiRmlUiViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Rml Ui Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  return pView->GetHandle();
}

void xiiRmlUiViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  xiiEngineProcessViewContext::SetCamera(pMsg);

  /*const xiiUInt32 viewHeight = pMsg->m_uiWindowHeight;

  auto hResource = m_pRmlUiContext->GetResource();
  if (hResource.IsValid())
  {
    xiiResourceLock<xiiRmlUiResource> pResource(hResource, xiiResourceAcquireMode::AllowLoadingFallback);

    if (pResource->GetDetails().m_Bounds.IsValid())
    {
      const xiiBoundingBox& bbox = pResource->GetDetails().m_Bounds.GetBox();

      xiiStringBuilder sText;
      sText.PrependFormat("Bounding Box: width={0}, depth={1}, height={2}", xiiArgF(bbox.GetHalfExtents().x * 2, 2),
                          xiiArgF(bbox.GetHalfExtents().y * 2, 2), xiiArgF(bbox.GetHalfExtents().z * 2, 2));

      xiiDebugRenderer::DrawInfoText(m_hView, sText, xiiVec2I32(10, viewHeight - 26), xiiColor::White);
    }
  }*/
}
