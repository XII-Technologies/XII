#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class XII_EDITORFRAMEWORK_DLL xiiDrawBoxGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiDrawBoxGizmo, xiiGizmo);

public:
  enum class ManipulateMode
  {
    None,
    DrawBase,
    DrawHeight,
  };

  xiiDrawBoxGizmo();
  ~xiiDrawBoxGizmo();

  void GetResult(xiiVec3& out_Origin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY, float& out_fSizeNegZ, float& out_fSizePosZ) const;

  ManipulateMode GetCurrentMode() const { return m_ManipulateMode; }
  const xiiVec3& GetStartPosition() const { return m_vFirstCorner; }

  virtual void UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow) override;

  bool GetDisplayGrid() const { return m_bDisplayGrid; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual xiiEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

private:
  void SwitchMode(bool bCancel);
  void UpdateBox();
  void DisableGrid(bool bControlPressed);
  void UpdateGrid(QMouseEvent* e);
  bool PickPosition(QMouseEvent* e);

  ManipulateMode       m_ManipulateMode;
  xiiEngineGizmoHandle m_hBox;

  xiiInt32   m_iHeightChange = 0;
  xiiVec2I32 m_vLastMousePos;
  xiiVec3    m_vCurrentPosition;
  xiiVec3    m_vFirstCorner;
  xiiVec3    m_vSecondCorner;
  xiiVec3    m_vUpAxis;
  xiiVec3    m_vLastStartPoint;
  float      m_fBoxHeight         = 0.5f;
  float      m_fOriginalBoxHeight = 0.5f;
  bool       m_bDisplayGrid       = false;
};
