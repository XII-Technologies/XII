/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

#include <QPoint>

class XII_EDITORFRAMEWORK_DLL xiiConeAngleGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConeAngleGizmo, xiiGizmo);

public:
  xiiConeAngleGizmo();

  void     SetAngle(xiiAngle angle);
  xiiAngle GetAngle() const { return m_Angle; }

  void SetRadius(float fRadius) { m_fRadius = fRadius; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

private:
  xiiTime m_LastInteraction;

  xiiVec2I32 m_vLastMousePos;

  xiiEngineGizmoHandle m_hConeAngle;

  enum class ManipulateMode
  {
    None,
    Angle,
  };

  ManipulateMode m_ManipulateMode;

  xiiAngle m_Angle;
  float    m_fRadius;
  float    m_fAngleScale;
};
