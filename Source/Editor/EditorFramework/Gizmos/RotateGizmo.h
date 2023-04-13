#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class XII_EDITORFRAMEWORK_DLL xiiRotateGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRotateGizmo, xiiGizmo);

public:
  xiiRotateGizmo();

  const xiiQuat& GetRotationResult() const { return m_qCurrentRotation; }

  virtual void UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

private:
  xiiEngineGizmoHandle m_hAxisX;
  xiiEngineGizmoHandle m_hAxisY;
  xiiEngineGizmoHandle m_hAxisZ;

  xiiQuat  m_qStartRotation;
  xiiQuat  m_qCurrentRotation;
  xiiAngle m_Rotation;

  xiiVec2I32 m_vLastMousePos;

  xiiTime m_LastInteraction;
  xiiVec3 m_vRotationAxis;
  xiiMat4 m_mInvViewProj;
  xiiVec2 m_vScreenTangent;
};
