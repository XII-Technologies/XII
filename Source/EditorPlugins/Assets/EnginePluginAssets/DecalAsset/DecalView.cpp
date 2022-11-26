#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiDecalViewContext::xiiDecalViewContext(xiiDecalContext* pDecalContext) :
  xiiEngineProcessViewContext(pDecalContext)
{
  m_pDecalContext = pDecalContext;
}

xiiDecalViewContext::~xiiDecalViewContext() {}

xiiViewHandle xiiDecalViewContext::CreateView()
{
  xiiView* pView = nullptr;
  xiiRenderWorld::CreateView("Decal Editor - View", pView);

  pView->SetRenderPipelineResource(CreateDefaultRenderPipeline());

  xiiEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
  pView->SetWorld(pDocumentContext->GetWorld());
  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
