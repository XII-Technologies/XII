#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct xiiGizmoEvent;

class xiiCapsuleManipulatorAdapter : public xiiManipulatorAdapter
{
public:
  xiiCapsuleManipulatorAdapter();
  ~xiiCapsuleManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void         GizmoEventHandler(const xiiGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  xiiCapsuleGizmo m_Gizmo;
};
