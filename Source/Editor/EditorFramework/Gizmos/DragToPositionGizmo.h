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

  bool IsPickingSelectedAllowed() const override { return false; }
  bool IsPickingGizmosAllowed() const override { return false; }

  /// True after an axis handle aligns to a mesh in this gesture. Later grid hits retain that orientation.
  bool ModifiesRotation() const { return m_bModifiesRotation; }

  void UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow) override;

protected:
  void DoFocusLost(bool bCancel) override;

  xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  void OnVisibleChanged(bool bVisible) override;
  void OnTransformationChanged(const xiiTransform& transform) override;

  xiiEngineGizmoHandle m_hBobble;
  xiiEngineGizmoHandle m_hAlignPX;
  xiiEngineGizmoHandle m_hAlignNX;
  xiiEngineGizmoHandle m_hAlignPY;
  xiiEngineGizmoHandle m_hAlignNY;
  xiiEngineGizmoHandle m_hAlignPZ;
  xiiEngineGizmoHandle m_hAlignNZ;

  bool    m_bModifiesRotation;
  xiiVec3 m_vStartPosition;
  xiiQuat m_qStartOrientation;
};
