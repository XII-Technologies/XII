#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ConeAngleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct xiiGizmoEvent;

class xiiConeAngleManipulatorAdapter : public xiiManipulatorAdapter
{
public:
  xiiConeAngleManipulatorAdapter();
  ~xiiConeAngleManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void         GizmoEventHandler(const xiiGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  xiiConeAngleGizmo m_Gizmo;
};
