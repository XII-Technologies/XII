/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class xiiSpotLightVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiSpotLightVisualizerAdapter();
  ~xiiSpotLightVisualizerAdapter() override;

protected:
  void Finalize() override;
  void Update() override;

  void UpdateGizmoTransform() override;

  float                m_fScale;
  float                m_fAngleScale;
  xiiEngineGizmoHandle m_hGizmo;
};
