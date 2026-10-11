/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneView/SceneView.h>
#include <GraphicsCore/Pipeline/RenderWorldModule.h>
#include <GraphicsCore/Pipeline/View.h>
#include <SharedPluginScene/Scene/SceneShapeIconPass.h>

xiiViewHandle xiiSceneViewContext::CreateView()
{
  auto* world = GetDocumentContext()->GetWorld();
  XII_LOCK(world->GetWriteMarker());
  auto pipeline = xiiSceneShapeIconPass::GetPipeline();
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
