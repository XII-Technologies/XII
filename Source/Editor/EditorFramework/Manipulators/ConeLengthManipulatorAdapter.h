#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/ConeLengthGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct xiiGizmoEvent;

class xiiConeLengthManipulatorAdapter : public xiiManipulatorAdapter
{
public:
  xiiConeLengthManipulatorAdapter();
  ~xiiConeLengthManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void         GizmoEventHandler(const xiiGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  xiiConeLengthGizmo m_Gizmo;
};
