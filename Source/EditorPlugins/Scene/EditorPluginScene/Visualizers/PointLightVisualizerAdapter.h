/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class xiiPointLightVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiPointLightVisualizerAdapter();
  ~xiiPointLightVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float                m_fScale;
  xiiEngineGizmoHandle m_hGizmo;
};
