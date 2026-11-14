/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class xiiPointLightVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiPointLightVisualizerAdapter();
  ~xiiPointLightVisualizerAdapter() override;

protected:
  void Finalize() override;
  void Update() override;

  void UpdateGizmoTransform() override;

  float                m_fScale;
  xiiEngineGizmoHandle m_hGizmo;
};
