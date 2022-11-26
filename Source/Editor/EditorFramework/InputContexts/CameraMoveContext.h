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

  const xiiVec3& GetOrbitPoint() const;
  void           SetOrbitPoint(const xiiVec3& vPos);

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

private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(xiiInt32 iSpeed);
  void ResetCursor();
  void SetCurrentMouseMode();
  void DeactivateIfLast();

  xiiVec3 m_vOrbitPoint;

  xiiVec2I32 m_vLastMousePos;

  bool  m_bRotateCamera;
  bool  m_bMoveCamera;
  bool  m_bMoveCameraInPlane;
  bool  m_bOrbitCamera;
  bool  m_bSlideForwards;
  bool  m_bPanOrbitPoint;
  float m_fSlideForwardsDistance;
  bool  m_bOpenMenuOnMouseUp;

  xiiCamera* m_pCamera;

  bool m_bRun;
  bool m_bSlowDown;
  bool m_bMoveForwards;
  bool m_bMoveBackwards;
  bool m_bMoveRight;
  bool m_bMoveLeft;
  bool m_bMoveUp;
  bool m_bMoveDown;
  bool m_bMoveForwardsInPlane;
  bool m_bMoveBackwardsInPlane;
  bool m_bDidMoveMouse[3]; // Left Click, Right Click, Middle Click

  bool m_bRotateLeft  = false;
  bool m_bRotateRight = false;
  bool m_bRotateUp    = false;
  bool m_bRotateDown  = false;

  xiiTime m_LastUpdate;
};
