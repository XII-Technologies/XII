/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/DecalAsset/DecalContext.h>
#include <EnginePluginAssets/DecalAsset/DecalView.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>

xiiDecalViewContext::xiiDecalViewContext(xiiDecalContext* pDecalContext) :
  xiiEngineProcessViewContext(pDecalContext)
{
  m_pDecalContext = pDecalContext;
}

xiiDecalViewContext::~xiiDecalViewContext() = default;

xiiViewHandle xiiDecalViewContext::CreateView()
{
  xiiRenderPipelineResourceHandle pipeline;
  if (ResolveRenderPipeline(xiiEditorRenderPipelineUsage::Main, pipeline).Failed())
    return {};
  xiiView* pView = nullptr;
  GetDocumentContext()->GetWorld()->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Decal Editor - View", pView);
  pView->SetRenderPipelineResource(pipeline);
  pView->SetCameraUsageHint(xiiCameraUsageHint::EditorView);


  pView->SetCamera(&m_Camera);
  return pView->GetHandle();
}
