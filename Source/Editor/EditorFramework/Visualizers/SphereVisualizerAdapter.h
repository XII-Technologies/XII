/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct xiiGizmoEvent;

class xiiSphereVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiSphereVisualizerAdapter();
  ~xiiSphereVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float                            m_fScale;
  xiiVec3                          m_vPositionOffset;
  xiiEngineGizmoHandle             m_hGizmo;
  xiiBitflags<xiiVisualizerAnchor> m_Anchor;
};
