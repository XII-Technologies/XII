/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

class XII_EDITORFRAMEWORK_DLL xiiScaleGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScaleGizmo, xiiGizmo);

public:
  xiiScaleGizmo();

  const xiiVec3& GetScalingResult() const { return m_vScalingResult; }

  virtual void UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

protected:
  xiiEngineGizmoHandle m_hAxisX;
  xiiEngineGizmoHandle m_hAxisY;
  xiiEngineGizmoHandle m_hAxisZ;
  xiiEngineGizmoHandle m_hAxisXYZ;

private:
  xiiVec3 m_vScalingResult;
  xiiVec3 m_vScaleMouseMove;

  xiiVec2I32 m_vLastMousePos;

  xiiTime m_LastInteraction;
  xiiVec3 m_vMoveAxis;
  xiiMat4 m_mInvViewProj;
};

/// Scale gizmo version that only uses boxes that can be composited with
/// rotate and translate gizmos without major overlap.
/// Used by the xiiTransformManipulatorAdapter.
class XII_EDITORFRAMEWORK_DLL xiiManipulatorScaleGizmo : public xiiScaleGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiManipulatorScaleGizmo, xiiScaleGizmo);

public:
  xiiManipulatorScaleGizmo();

protected:
  virtual void OnTransformationChanged(const xiiTransform& transform) override;
};
