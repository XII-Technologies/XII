#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct xiiGizmoEvent;

class xiiNonUniformBoxManipulatorAdapter : public xiiManipulatorAdapter
{
public:
  xiiNonUniformBoxManipulatorAdapter();
  ~xiiNonUniformBoxManipulatorAdapter();

  virtual void QueryGridSettings(xiiGridSettingsMsgToEngine& outGridSettings) override;

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void         GizmoEventHandler(const xiiGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  xiiNonUniformBoxGizmo m_Gizmo;
};
