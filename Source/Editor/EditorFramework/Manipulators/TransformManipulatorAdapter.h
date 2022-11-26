#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Manipulators/ManipulatorAdapter.h>

struct xiiGizmoEvent;

class xiiTransformManipulatorAdapter : public xiiManipulatorAdapter
{
public:
  xiiTransformManipulatorAdapter();
  ~xiiTransformManipulatorAdapter();

protected:
  virtual void Finalize() override;
  virtual void Update() override;
  void         GizmoEventHandler(const xiiGizmoEvent& e);

  virtual void UpdateGizmoTransform() override;

  xiiVec3 GetTranslation();
  xiiQuat GetRotation();
  xiiVec3 GetScale();

  virtual xiiTransform GetOffsetTransform() const override;

  xiiTranslateGizmo        m_TranslateGizmo;
  xiiRotateGizmo           m_RotateGizmo;
  xiiManipulatorScaleGizmo m_ScaleGizmo;
  xiiVec3                  m_vOldScale;

  bool m_bHideTranslate = true;
  bool m_bHideRotate    = true;
  bool m_bHideScale     = true;
};
