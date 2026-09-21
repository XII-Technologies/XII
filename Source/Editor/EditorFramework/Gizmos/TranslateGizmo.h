/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

#include <QPoint>

class XII_EDITORFRAMEWORK_DLL xiiTranslateGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTranslateGizmo, xiiGizmo);

public:
  xiiTranslateGizmo();

  const xiiVec3 GetStartPosition() const { return m_vStartPosition; }
  const xiiVec3 GetTranslationResult() const { return GetTransformation().m_vPosition - m_vStartPosition; }
  const xiiVec3 GetTranslationDiff() const { return m_vLastMoveDiff; }

  enum class MovementMode
  {
    ScreenProjection,
    MouseDiff
  };

  enum class HandleInteraction
  {
    None,
    AxisX,
    AxisY,
    AxisZ,
    PlaneX,
    PlaneY,
    PlaneZ,
  };

  enum class TranslateMode
  {
    None,
    Axis,
    Plane
  };

  void              SetMovementMode(MovementMode mode);
  HandleInteraction GetLastHandleInteraction() const { return m_LastHandleInteraction; }
  TranslateMode     GetTranslateMode() const { return m_Mode; }

  /// Used when CTRL+drag moves the object AND the camera
  void SetCameraSpeed(float fSpeed);

  virtual void UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

  xiiResult GetPointOnAxis(xiiInt32 iScreenPosX, xiiInt32 iScreenPosY, xiiVec3& out_Result) const;
  xiiResult GetPointOnPlane(xiiInt32 iScreenPosX, xiiInt32 iScreenPosY, xiiVec3& out_Result) const;

private:
  xiiVec2I32 m_vLastMousePos;
  xiiVec2    m_vTotalMouseDiff;

  xiiVec3 m_vLastMoveDiff;

  MovementMode         m_MovementMode;
  xiiEngineGizmoHandle m_hAxisX;
  xiiEngineGizmoHandle m_hAxisY;
  xiiEngineGizmoHandle m_hAxisZ;

  xiiEngineGizmoHandle m_hPlaneXY;
  xiiEngineGizmoHandle m_hPlaneXZ;
  xiiEngineGizmoHandle m_hPlaneYZ;

  TranslateMode     m_Mode;
  HandleInteraction m_LastHandleInteraction;

  float m_fStartScale;
  float m_fCameraSpeed;

  xiiTime m_LastInteraction;
  xiiVec3 m_vMoveAxis;
  xiiVec3 m_vPlaneAxis[2];
  xiiVec3 m_vStartPosition;
  xiiMat4 m_mInvViewProj;
};
