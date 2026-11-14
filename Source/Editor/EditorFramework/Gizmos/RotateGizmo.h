/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class XII_EDITORFRAMEWORK_DLL xiiRotateGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRotateGizmo, xiiGizmo);

public:
  xiiRotateGizmo();

  const xiiQuat& GetRotationResult() const { return m_qCurrentRotation; }

  void UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow) override;

protected:
  void DoFocusLost(bool bCancel) override;

  xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  void OnVisibleChanged(bool bVisible) override;
  void OnTransformationChanged(const xiiTransform& transform) override;

private:
  xiiEngineGizmoHandle m_hAxisX;
  xiiEngineGizmoHandle m_hAxisY;
  xiiEngineGizmoHandle m_hAxisZ;

  xiiQuat  m_qStartRotation;
  xiiQuat  m_qCurrentRotation;
  xiiAngle m_Rotation;
  float    m_fArcStart = 0;

  xiiVec2I32 m_vLastMousePos;

  xiiVec3 m_vRotationAxis;
  xiiMat4 m_mInvViewProj;
  xiiVec2 m_vScreenTangent;
};
