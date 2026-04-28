/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

#include <QPoint>

class XII_EDITORFRAMEWORK_DLL xiiCapsuleGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCapsuleGizmo, xiiGizmo);

public:
  xiiCapsuleGizmo();

  void SetLength(float fRadius);
  void SetRadius(float fLength);

  float GetLength() const { return m_fLength; }
  float GetRadius() const { return m_fRadius; }

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

  xiiEngineGizmoHandle m_hLengthTop;
  xiiEngineGizmoHandle m_hLengthBottom;
  xiiEngineGizmoHandle m_hRadius;

  enum class ManipulateMode
  {
    None,
    Length,
    Radius,
  };

  ManipulateMode m_ManipulateMode;

  float m_fRadius;
  float m_fLength;
};
