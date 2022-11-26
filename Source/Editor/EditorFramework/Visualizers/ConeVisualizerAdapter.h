#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct xiiGizmoEvent;

class xiiConeVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiConeVisualizerAdapter();
  ~xiiConeVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  float                m_fFinalScale;
  float                m_fAngleScale;
  xiiEngineGizmoHandle m_hGizmo;
};
