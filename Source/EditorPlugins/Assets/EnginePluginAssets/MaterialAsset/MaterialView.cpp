/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MaterialAsset/MaterialContext.h>
#include <EnginePluginAssets/MaterialAsset/MaterialView.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

xiiMaterialViewContext::xiiMaterialViewContext(xiiMaterialContext* pMaterialContext) :
  xiiEngineProcessViewContext(pMaterialContext)
{
  m_pMaterialContext = pMaterialContext;
}

xiiMaterialViewContext::~xiiMaterialViewContext() = default;

void xiiMaterialViewContext::PositionThumbnailCamera()
{
  m_Camera.SetCameraMode(xiiCameraMode::PerspectiveFixedFovX, 45.0f, 0.1f, 1000.0f);
  m_Camera.LookAt(xiiVec3(+0.23f, -0.23f, 0.1f), xiiVec3::MakeZero(), xiiVec3(0.0f, 0.0f, 1.0f));
}

xiiViewHandle xiiMaterialViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Material Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());
  pView->SetShaderPermutationVariable("MATERIAL_PREVIEW", "TRUE");

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
