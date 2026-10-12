/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <EditorPluginScene/EditorPluginSceneDLL.h>

/// Native reflection influence volume and inner full-weight boundary, in the probe's local space.
class XII_EDITORPLUGINSCENE_DLL xiiReflectionCaptureVisualizerAdapter : public xiiVisualizerAdapter
{
protected:
  void Finalize() override;
  void Update() override;
  void UpdateGizmoTransform() override;

  xiiEngineGizmoHandle m_hBox;
  xiiEngineGizmoHandle m_hInnerBox;
  xiiEngineGizmoHandle m_hSphere;
  xiiEngineGizmoHandle m_hInnerSphere;
  xiiVec3              m_vOuterScale = xiiVec3::MakeZero();
  xiiVec3              m_vInnerScale = xiiVec3::MakeZero();
};
