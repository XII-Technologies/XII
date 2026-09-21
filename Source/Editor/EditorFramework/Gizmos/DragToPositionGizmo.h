/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Math/Quat.h>

class XII_EDITORFRAMEWORK_DLL xiiDragToPositionGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDragToPositionGizmo, xiiGizmo);

public:
  xiiDragToPositionGizmo();

  const xiiVec3 GetTranslationResult() const { return GetTransformation().m_vPosition - m_vStartPosition; }
  const xiiQuat GetRotationResult() const { return GetTransformation().m_qRotation; }

  virtual bool IsPickingSelectedAllowed() const override { return false; }

  /// Returns true if any of the 'align with' handles is selected, and thus the rotation of the dragged object should be modified as well
  bool ModifiesRotation() const { return m_bModifiesRotation; }

  virtual void UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

  xiiEngineGizmoHandle m_hBobble;
  xiiEngineGizmoHandle m_hAlignPX;
  xiiEngineGizmoHandle m_hAlignNX;
  xiiEngineGizmoHandle m_hAlignPY;
  xiiEngineGizmoHandle m_hAlignNY;
  xiiEngineGizmoHandle m_hAlignPZ;
  xiiEngineGizmoHandle m_hAlignNZ;

  bool    m_bModifiesRotation;
  xiiTime m_LastInteraction;
  xiiVec3 m_vStartPosition;
  xiiQuat m_qStartOrientation;
};
