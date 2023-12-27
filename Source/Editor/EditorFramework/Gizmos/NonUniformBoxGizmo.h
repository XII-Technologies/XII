#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <QPoint>

class XII_EDITORFRAMEWORK_DLL xiiNonUniformBoxGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiNonUniformBoxGizmo, xiiGizmo);

public:
  xiiNonUniformBoxGizmo();

  void SetSize(const xiiVec3& vNegSize, const xiiVec3& vPosSize, bool bLinkAxis = false);

  const xiiVec3& GetNegSize() const { return m_vNegSize; }
  const xiiVec3& GetPosSize() const { return m_vPosSize; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

private:
  xiiResult GetPointOnAxis(xiiInt32 iScreenPosX, xiiInt32 iScreenPosY, xiiVec3& out_Result) const;

  xiiTime m_LastInteraction;
  xiiMat4 m_mInvViewProj;

  xiiVec2I32 m_vLastMousePos;

  xiiEngineGizmoHandle m_hOutline;
  xiiEngineGizmoHandle m_Nobs[6];
  xiiVec3              m_vMainAxis[6];

  enum ManipulateMode
  {
    None = -1,
    DragNegX,
    DragPosX,
    DragNegY,
    DragPosY,
    DragNegZ,
    DragPosZ,
  };

  ManipulateMode m_ManipulateMode = ManipulateMode::None;

  xiiVec3 m_vNegSize;
  xiiVec3 m_vPosSize;
  xiiVec3 m_vStartNegSize;
  xiiVec3 m_vStartPosSize;
  xiiVec3 m_vMoveAxis;
  xiiVec3 m_vStartPosition;
  xiiVec3 m_vInteractionPivot;
  float   m_fStartScale = 1.0f;
  bool    m_bLinkAxis   = false;
};
