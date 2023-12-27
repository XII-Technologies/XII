#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Visualizers/VisualizerAdapter.h>

struct xiiGizmoEvent;

class xiiDirectionVisualizerAdapter : public xiiVisualizerAdapter
{
public:
  xiiDirectionVisualizerAdapter();
  ~xiiDirectionVisualizerAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;

  virtual void UpdateGizmoTransform() override;

  xiiEngineGizmoHandle m_hGizmo;
};
