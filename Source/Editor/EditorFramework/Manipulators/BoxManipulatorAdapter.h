#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct xiiGizmoEvent;

class xiiBoxManipulatorAdapter : public xiiManipulatorAdapter
{
public:
  xiiBoxManipulatorAdapter();
  ~xiiBoxManipulatorAdapter();

  virtual void QueryGridSettings(xiiGridSettingsMsgToEngine& out_gridSettings) override;

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void         GizmoEventHandler(const xiiGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  xiiVec3               m_vPositionOffset;
  xiiQuat               m_qRotation;
  xiiNonUniformBoxGizmo m_Gizmo;

  xiiVec3 m_vOldSize;
};
