/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/InputContexts/EditorInputContext.h>

class xiiCamera;

/// A simple orbit camera. Use LMB to rotate, wheel to zoom, Alt to slow down.
class XII_EDITORFRAMEWORK_DLL xiiOrbitCameraContext : public xiiEditorInputContext
{
public:
  xiiOrbitCameraContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView);

  void       SetCamera(xiiCamera* pCamera);
  xiiCamera* GetCamera() const;

  void SetDefaultCameraRelative(const xiiVec3& vDirection, float fDistanceScale);
  void SetDefaultCameraFixed(const xiiVec3& vPosition);

  void MoveCameraToDefaultPosition();

  /// Defines the box in which the user may move the camera around
  void SetOrbitVolume(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize);

  /// The center point around which the camera can be moved and rotated.
  xiiVec3 GetVolumeCenter() const { return m_Volume.GetCenter(); }

  /// The half-size of the volume in which the camera may move around
  xiiVec3 GetVolumeHalfSize() const { return m_Volume.GetHalfExtents(); }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoWheelEvent(QWheelEvent* e) override;
  virtual xiiEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual xiiEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override;

  float GetCameraSpeed() const;

  void ResetCursor();
  void SetCurrentMouseMode();

  xiiVec2I32 m_vLastMousePos;

  enum class Mode
  {
    Off,
    Orbit,
    Free,
    Pan,
  };

  Mode       m_Mode = Mode::Off;
  xiiCamera* m_pCamera;

  xiiBoundingBox m_Volume;

  bool    m_bFixedDefaultCamera = true;
  xiiVec3 m_vDefaultCamera      = xiiVec3(1, 0, 0);

  bool m_bRun           = false;
  bool m_bMoveForwards  = false;
  bool m_bMoveBackwards = false;
  bool m_bMoveRight     = false;
  bool m_bMoveLeft      = false;
  bool m_bMoveUp        = false;
  bool m_bMoveDown      = false;

  xiiTime m_LastUpdate;
};
