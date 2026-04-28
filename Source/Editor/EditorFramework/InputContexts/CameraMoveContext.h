/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Time/Time.h>

#include <QPoint>

class xiiCamera;

class XII_EDITORFRAMEWORK_DLL xiiCameraMoveContext : public xiiEditorInputContext
{
public:
  xiiCameraMoveContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView);

  void LoadState();

  void SetCamera(xiiCamera* pCamera);

  xiiVec3 GetOrbitPoint() const;
  void    SetOrbitDistance(float fDistance);

  static float ConvertCameraSpeed(xiiUInt32 uiSpeedIdx);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual xiiEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;
  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoWheelEvent(QWheelEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override {}

  void OnActivated() override;

private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(xiiInt32 iSpeed);
  void ResetCursor();
  void SetCurrentMouseMode();
  void DeactivateIfLast();

  float m_fOrbitPointDistance = 1.0f;

  xiiVec2I32 m_vLastMousePos  = xiiVec2I32::MakeZero();
  xiiVec2I32 m_vMouseClickPos = xiiVec2I32::MakeZero();

  bool m_bRotateCamera      = false;
  bool m_bMoveCamera        = false;
  bool m_bMoveCameraInPlane = false;
  bool m_bOrbitCamera       = false;
  bool m_bSlideForwards     = false;
  bool m_bPanOrbitPoint     = false;
  bool m_bPanCamera         = false;
  bool m_bOpenMenuOnMouseUp = false;

  xiiCamera* m_pCamera = nullptr;

  bool     m_bRun                  = false;
  bool     m_bSlowDown             = false;
  bool     m_bMoveForwards         = false;
  bool     m_bMoveBackwards        = false;
  bool     m_bMoveRight            = false;
  bool     m_bMoveLeft             = false;
  bool     m_bMoveUp               = false;
  bool     m_bMoveDown             = false;
  bool     m_bMoveForwardsInPlane  = false;
  bool     m_bMoveBackwardsInPlane = false;
  xiiInt32 m_iDidMoveMouse[3]      = {0, 0, 0}; // Left Click, Right Click, Middle Click

  bool m_bRotateLeft  = false;
  bool m_bRotateRight = false;
  bool m_bRotateUp    = false;
  bool m_bRotateDown  = false;

  xiiTime m_LastUpdate;
};
