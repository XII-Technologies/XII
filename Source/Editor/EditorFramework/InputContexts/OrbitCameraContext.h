#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class xiiCamera;

/// \brief A simple orbit camera. Use LMB to rotate, wheel to zoom, Alt to slow down.
class XII_EDITORFRAMEWORK_DLL xiiOrbitCameraContext : public xiiEditorInputContext
{
public:
  xiiOrbitCameraContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView);

  void       SetCamera(xiiCamera* pCamera);
  xiiCamera* GetCamera() const;

  /// \brief Defines the box in which the user may move the camera around
  void SetOrbitVolume(const xiiVec3& vCenterPos, const xiiVec3& vHalfBoxSize, const xiiVec3& vDefaultCameraPosition, bool bSetCamLookat);

  /// \brief The center point around which the camera can be moved and rotated.
  xiiVec3 GetVolumeCenter() const { return m_Volume.GetCenter(); }

  /// \brief The half-size of the volume in which the camera may move around
  xiiVec3 GetVolumeHalfSize() const { return m_Volume.GetHalfExtents(); }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoWheelEvent(QWheelEvent* e) override;
  virtual xiiEditorInput DoKeyPressEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override {}



private:
  virtual void UpdateContext() override{};

  void ResetCursor();
  void SetCurrentMouseMode();

  xiiVec2I32 m_vLastMousePos;

  enum class Mode
  {
    Off,
    Orbit,
    UpDown,
    MovePlane,
    Pan,
  };

  Mode       m_Mode;
  xiiCamera* m_pCamera;

  xiiVec3 m_vDefaultCameraPosition;
  xiiVec3 m_vOrbitPoint;

  xiiBoundingBox m_Volume;
};
