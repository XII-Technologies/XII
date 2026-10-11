/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/SceneView/SceneView.h>
#include <GraphicsCore/Components/Render/CameraComponent.h>
#include <GraphicsCore/Pipeline/View.h>

void xiiSceneViewContext::UpdateCameraPreview(bool bRenderEditorGizmos)
{
  XII_LOCK(m_pSceneContext->GetWorld()->GetWriteMarker());
  m_CameraPreview.SuspendRendering();
  auto* view = GetView();
  if (!view || !bRenderEditorGizmos || !m_pSceneContext->GetRenderSelectionBoxes() || view->GetCameraUsageHint() != xiiCameraUsageHint::EditorView)
    return;

  for (const auto handle : m_pSceneContext->GetSelectionWithChildren())
  {
    const xiiGameObject* object = nullptr;
    if (!view->GetWorld()->TryGetObject(handle, object) || !object->IsActive())
      continue;
    const auto& tags = object->GetTags();
    if (tags.IsAnySet(view->m_ExcludeTags) || (!view->m_IncludeTags.IsEmpty() && !tags.IsAnySet(view->m_IncludeTags)))
      continue;
    const xiiCameraComponent* camera = nullptr;
    if (object->TryGetComponentOfBaseType(camera) && camera->IsActiveAndInitialized())
    {
      if (m_CameraPreview.Update(*view, *camera).Failed())
        xiiLog::Warning("Could not update the selected camera preview.");
      break;
    }
  }
}

void xiiSceneViewContext::SuspendRendering()
{
  m_CameraPreview.SuspendRendering();
  xiiEngineProcessViewContext::SuspendRendering();
}
