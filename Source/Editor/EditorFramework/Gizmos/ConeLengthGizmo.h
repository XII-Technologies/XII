#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <QPoint>

class XII_EDITORFRAMEWORK_DLL xiiConeLengthGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConeLengthGizmo, xiiGizmo);

public:
  xiiConeLengthGizmo();

  void  SetRadius(float fRadius);
  float GetRadius() const { return m_fRadius; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const xiiTransform& transform) override;

private:
  xiiTime m_LastInteraction;

  xiiVec2I32 m_vLastMousePos;

  xiiEngineGizmoHandle m_hConeRadius;

  enum class ManipulateMode
  {
    None,
    Radius
  };

  ManipulateMode m_ManipulateMode;

  float m_fRadius;
  float m_fRadiusScale;
};
