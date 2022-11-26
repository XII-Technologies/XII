#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class xiiBoxReflectionProbeVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiBoxReflectionProbeVisualizerAdapter();
  ~xiiBoxReflectionProbeVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  xiiVec3 m_vScale;
  xiiVec3 m_vPositionOffset;
  xiiQuat m_qRotation;

  xiiEngineGizmoHandle m_hGizmo;
};
