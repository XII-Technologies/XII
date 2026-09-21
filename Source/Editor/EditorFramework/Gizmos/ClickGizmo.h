/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>

/// The click gizmo displays a simple shape that can be clicked.
///
/// This can be used to provide the user with a way to select which part to edit further.
class XII_EDITORFRAMEWORK_DLL xiiClickGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiClickGizmo, xiiGizmo);

public:
  xiiClickGizmo();

  void SetColor(const xiiColor& color);

protected:
  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual void DoFocusLost(bool bCancel) override;
  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

private:
  xiiEngineGizmoHandle m_hShape;
};
