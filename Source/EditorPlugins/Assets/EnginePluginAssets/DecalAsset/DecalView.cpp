#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

xiiDecalViewContext::xiiDecalViewContext(xiiDecalContext* pDecalContext) :
  xiiEngineProcessViewContext(pDecalContext)
{
  m_pDecalContext = pDecalContext;
}

xiiDecalViewContext::~xiiDecalViewContext() = default;

xiiViewHandle xiiDecalViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Decal Editor - View", pView);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
