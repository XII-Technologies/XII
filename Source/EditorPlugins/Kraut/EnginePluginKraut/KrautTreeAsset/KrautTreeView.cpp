#include <EnginePluginKraut/EnginePluginKrautPCH.h>

#include <EnginePluginKraut/KrautTreeAsset/KrautTreeContext.h>
#include <EnginePluginKraut/KrautTreeAsset/KrautTreeView.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiKrautTreeViewContext::xiiKrautTreeViewContext(xiiKrautTreeContext* pKrautTreeContext) :
  xiiEngineProcessViewContext(pKrautTreeContext)
{
  m_pKrautTreeContext = pKrautTreeContext;

  // Start with something valid.
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.05f, 10000.0f);
  m_Camera.LookAt(xiiVec3(1, 1, 1), xiiVec3::ZeroVector(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiKrautTreeViewContext::~xiiKrautTreeViewContext() {}

bool xiiKrautTreeViewContext::UpdateThumbnailCamera(const xiiBoundingBoxSphere& bounds)
{
  return !FocusCameraOnObject(m_Camera, bounds, 45.0f, -xiiVec3(5, -2, 3));
}

xiiViewHandle xiiKrautTreeViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Kraut Tree Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  pView->SetCameraUsageHint(xiiCameraUsageHint::Thumbnail);
  return pView->GetHandle();
}

void xiiKrautTreeViewContext::SetCamera(const xiiViewRedrawMsgToEngine* pMsg)
{
  xiiEngineProcessViewContext::SetCamera(pMsg);

  const xiiUInt32 viewHeight = pMsg->m_uiWindowHeight;

  xiiBoundingBox bbox;
  bbox.SetCenterAndHalfExtents(xiiVec3::ZeroVector(), xiiVec3::ZeroVector());

  auto hResource = m_pKrautTreeContext->GetResource();
  if (hResource.IsValid())
  {
    //xiiResourceLock<xiiKrautGeneratorResource> pResource(hResource, xiiResourceAcquireMode::AllowLoadingFallback);

    // TODO

    //if (pResource->GetDetails().m_Bounds.IsValid())
    //{
    //  bbox = pResource->GetDetails().m_Bounds.GetBox();

    //  xiiStringBuilder sText;
    //  sText.PrependFormat("Bounding Box: width={0}, depth={1}, height={2}", xiiArgF(bbox.GetHalfExtents().x * 2, 2),
    //    xiiArgF(bbox.GetHalfExtents().y * 2, 2), xiiArgF(bbox.GetHalfExtents().z * 2, 2));

    //  xiiDebugRenderer::Draw2DText(m_hView, sText, xiiVec2I32(10, viewHeight - 26), xiiColor::White);
    //}
  }
}
