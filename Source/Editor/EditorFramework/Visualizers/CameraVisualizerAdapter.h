#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

class xiiCameraVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiCameraVisualizerAdapter();
  ~xiiCameraVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  xiiTransform         m_LocalTransformFrustum;
  xiiTransform         m_LocalTransformNearPlane;
  xiiTransform         m_LocalTransformFarPlane;
  xiiEngineGizmoHandle m_hBoxGizmo;
  xiiEngineGizmoHandle m_hFrustumGizmo;
  xiiEngineGizmoHandle m_hNearPlaneGizmo;
  xiiEngineGizmoHandle m_hFarPlaneGizmo;
};
