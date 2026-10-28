/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <QPointF>

/// Per-view navigation input; independent of scene selection and transform handles.
class XII_EDITORFRAMEWORK_DLL xiiEditorViewportNavigationGizmo : public xiiEditorInputContext
{
public:
  xiiEditorViewportNavigationGizmo(xiiQtEngineDocumentWindow* pWindow, xiiQtEngineViewWidget* pView);
  xiiInt32 GetHoveredAxis() const;
  bool     IsHovered() const;

protected:
  void           OnSetOwner(xiiQtEngineDocumentWindow*, xiiQtEngineViewWidget*) override {}
  void           DoFocusLost(bool bCancel) override;
  xiiEditorInput DoMousePressEvent(QMouseEvent* e) override;
  xiiEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  xiiEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  xiiEditorInput DoWheelEvent(QWheelEvent* e) override;

private:
  QPointF  m_PressPosition;
  QPointF  m_LastPosition;
  xiiInt32 m_iPressedAxis = -1;
  bool     m_bDragging    = false;
};
