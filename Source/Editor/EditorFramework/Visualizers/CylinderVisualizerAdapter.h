#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct xiiGizmoEvent;

class xiiCylinderVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiCylinderVisualizerAdapter();
  ~xiiCylinderVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float                            m_fRadius;
  float                            m_fHeight;
  xiiVec3                          m_vPositionOffset;
  xiiBitflags<xiiVisualizerAnchor> m_Anchor;
  xiiBasisAxis::Enum               m_Axis;

  xiiEngineGizmoHandle m_hCylinder;
};
