/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct xiiGizmoEvent;

class xiiCapsuleVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiCapsuleVisualizerAdapter();
  ~xiiCapsuleVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float                            m_fRadius = 0.0f;
  float                            m_fHeight = 0.0f;
  xiiBitflags<xiiVisualizerAnchor> m_Anchor;

  xiiEngineGizmoHandle m_hSphereTop;
  xiiEngineGizmoHandle m_hSphereBottom;
  xiiEngineGizmoHandle m_hCylinder;
};
