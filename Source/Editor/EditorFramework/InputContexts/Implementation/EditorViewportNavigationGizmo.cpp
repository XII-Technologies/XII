/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/Rendering/ViewportNavigationGizmo.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorViewportNavigationGizmo.h>
#include <QApplication>
#include <QMouseEvent>
#include <QWheelEvent>

xiiEditorViewportNavigationGizmo::xiiEditorViewportNavigationGizmo(xiiQtEngineDocumentWindow* pWindow, xiiQtEngineViewWidget* pView)
{
  SetOwner(pWindow, pView);
  SetShortcutsDisabled(true);
}

bool xiiEditorViewportNavigationGizmo::IsHovered() const
{
  const auto* view = GetOwnerView();
  if (!view->IsNavigationGizmoEnabled() || (IsAnyInputContextActive() && !IsActiveInputContext()))
    return false;
  const QPoint position = view->mapFromGlobal(QCursor::pos());
  return view->rect().contains(position) && view->GetNavigationGizmoGeometry().Contains(xiiVec2(position.x(), position.y()));
}

xiiInt32 xiiEditorViewportNavigationGizmo::GetHoveredAxis() const
{
  if (!IsHovered())
    return -1;
  const QPoint position = GetOwnerView()->mapFromGlobal(QCursor::pos());
  return GetOwnerView()->GetNavigationGizmoGeometry().HitTest(xiiVec2(position.x(), position.y()));
}

void xiiEditorViewportNavigationGizmo::DoFocusLost(bool bCancel)
{
  m_iPressedAxis = -1;
  m_bDragging    = false;
  if (IsActiveInputContext())
  {
    GetOwnerView()->releaseMouse();
    MakeActiveInputContext(false);
  }
}

xiiEditorInput xiiEditorViewportNavigationGizmo::DoMousePressEvent(QMouseEvent* e)
{
  const auto    geometry = GetOwnerView()->GetNavigationGizmoGeometry();
  const xiiVec2 position(static_cast<float>(e->position().x()), static_cast<float>(e->position().y()));
  if (!GetOwnerView()->IsNavigationGizmoEnabled() || IsAnyInputContextActive() || !geometry.Contains(position))
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() == Qt::LeftButton)
  {
    m_PressPosition = m_LastPosition = e->position();
    m_iPressedAxis                   = geometry.HitTest(position);
    m_bDragging                      = false;
    GetOwnerView()->setFocus(Qt::MouseFocusReason);
    GetOwnerView()->CancelCameraInterpolation();
    MakeActiveInputContext();
    GetOwnerView()->grabMouse();
  }
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiEditorViewportNavigationGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return GetOwnerView()->IsNavigationGizmoEnabled() && GetOwnerView()->GetNavigationGizmoGeometry().Contains(xiiVec2(e->position().x(), e->position().y())) ? xiiEditorInput::WasExclusivelyHandled : xiiEditorInput::MayBeHandledByOthers;

  if (!m_bDragging && (e->position() - m_PressPosition).manhattanLength() >= QApplication::startDragDistance())
    m_bDragging = true;
  if (m_bDragging)
  {
    const QPointF delta = e->position() - m_LastPosition;
    GetOwnerView()->OrbitCamera(xiiVec2(delta.x() * 0.01f, -delta.y() * 0.01f));
  }
  m_LastPosition = m_bDragging ? e->position() : m_PressPosition;
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiEditorViewportNavigationGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;
  if (e->button() == Qt::LeftButton)
  {
    const auto geometry = GetOwnerView()->GetNavigationGizmoGeometry();
    if (!m_bDragging && m_iPressedAxis >= 0 && geometry.HitTest(xiiVec2(e->position().x(), e->position().y())) == m_iPressedAxis)
    {
      xiiVec3 direction = -xiiViewportNavigationGizmo::GetAxisDirection(m_iPressedAxis);
      if (GetOwnerView()->m_pViewConfig->m_Camera.GetCenterDirForwards().Dot(direction) > 0.9999f)
        direction = -direction;
      GetOwnerView()->InterpolateCameraOrbitTo(direction);
    }
    DoFocusLost(false);
  }
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiEditorViewportNavigationGizmo::DoWheelEvent(QWheelEvent* e)
{
  return IsActiveInputContext() || (GetOwnerView()->IsNavigationGizmoEnabled() && GetOwnerView()->GetNavigationGizmoGeometry().Contains(xiiVec2(e->position().x(), e->position().y()))) ? xiiEditorInput::WasExclusivelyHandled : xiiEditorInput::MayBeHandledByOthers;
}
