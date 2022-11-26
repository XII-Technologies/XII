#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class XII_EDITORFRAMEWORK_DLL xiiBoxGizmo : public xiiGizmo
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoxGizmo, xiiGizmo);

public:
  xiiBoxGizmo();

  void SetSize(const xiiVec3& size);

  const xiiVec3& GetSize() const { return m_vSize; }

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

  xiiEngineGizmoHandle m_hCorners;
  xiiEngineGizmoHandle m_Edges[3];
  xiiEngineGizmoHandle m_Faces[3];

  enum class ManipulateMode
  {
    None,
    Uniform,
    AxisX,
    AxisY,
    AxisZ,
    PlaneXY,
    PlaneXZ,
    PlaneYZ,
  };

  ManipulateMode m_ManipulateMode;

  xiiVec3 m_vSize;
};
