#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiMaterialViewContext::xiiMaterialViewContext(xiiMaterialContext* pMaterialContext) :
  xiiEngineProcessViewContext(pMaterialContext)
{
  m_pMaterialContext = pMaterialContext;
}

xiiMaterialViewContext::~xiiMaterialViewContext() {}

void xiiMaterialViewContext::PositionThumbnailCamera()
{
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(xiiVec3(+0.23f, -0.23f, 0.1f), xiiVec3::ZeroVector(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiViewHandle xiiMaterialViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Material Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
  pView->SetShaderPermutationVariable("MATERIAL_PREVIEW", "TRUE");

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
