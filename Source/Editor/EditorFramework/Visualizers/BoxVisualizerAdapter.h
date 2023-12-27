#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct xiiGizmoEvent;

class xiiBoxVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiBoxVisualizerAdapter();
  ~xiiBoxVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  xiiVec3                          m_vScale;
  xiiVec3                          m_vPositionOffset;
  xiiQuat                          m_qRotation;
  xiiBitflags<xiiVisualizerAnchor> m_Anchor;
  xiiEngineGizmoHandle             m_hGizmo;
};
