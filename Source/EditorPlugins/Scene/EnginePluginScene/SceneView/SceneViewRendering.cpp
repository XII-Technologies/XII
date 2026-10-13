/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <EnginePluginScene/SceneView/SceneView.h>
#include <Foundation/Configuration/CVar.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <SharedPluginScene/Scene/SceneCameraSettings.h>
#include <SharedPluginScene/Scene/SceneRenderPipeline.h>

namespace
{
  xiiCVarBool cvarSceneCullingCamera("Editor.Scene.UseCullingCamera", false, xiiCVarFlags::Default, "Cull Editor scene views using the active camera component whose usage is CullingOnly.");
}

xiiViewHandle xiiSceneViewContext::CreateView()
{
  auto* world = GetDocumentContext()->GetWorld();
  XII_LOCK(world->GetWriteMarker());
  auto pipeline = xiiSceneRenderPipeline::GetDefaultResource();
  if (!pipeline.IsValid())
  {
    xiiLog::Error("Could not create the native Editor scene pipeline resource.");
    return {};
  }
  xiiView* view = nullptr;
  world->GetOrCreateModule<xiiRenderWorldModule>()->CreateView("Editor - Scene", view);
  view->SetRenderPipelineResource(pipeline);
  view->SetCamera(&m_Camera);
  view->SetCullingCamera(&m_CullingCamera);
  view->SetCameraUsageHint(xiiCameraUsageHint::EditorView);
  view->SetPipelineParameter("Editor.ShapeIcons", m_pSceneContext->GetRenderShapeIcons());
  view->m_ExcludeTags.Set(xiiTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden"));
  return view->GetHandle();
}

void xiiSceneViewContext::SetupRenderTarget(xiiSharedPtr<xiiGALSwapChain> pSwapChain, xiiGALTextureView* pRenderTargetView, xiiUInt16 uiWidth, xiiUInt16 uiHeight)
{
  xiiEngineProcessViewContext::SetupRenderTarget(pSwapChain, pRenderTargetView, uiWidth, uiHeight);
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  if (xiiView* view = GetView())
    for (const xiiTag& tag : m_pSceneContext->GetInvisibleLayerTags())
      view->m_ExcludeTags.Set(tag);
}

void xiiSceneViewContext::SetInvisibleLayerTags(const xiiArrayPtr<xiiTag> removeTags, const xiiArrayPtr<xiiTag> addTags)
{
  XII_LOCK(GetDocumentContext()->GetWorld()->GetWriteMarker());
  if (xiiView* view = GetView())
  {
    for (const xiiTag& tag : removeTags)
      view->m_ExcludeTags.Remove(tag);
    for (const xiiTag& tag : addTags)
      view->m_ExcludeTags.Set(tag);
  }
}

void xiiSceneViewContext::UpdateCullingCamera()
{
  m_CullingCamera = m_Camera;
  if (const xiiView* view = GetView())
    if (xiiConfigureSceneCullingCamera(*view, cvarSceneCullingCamera, m_CullingCamera).Failed())
      xiiLog::Warning("Invalid scene culling camera; using the viewport camera.");
}
