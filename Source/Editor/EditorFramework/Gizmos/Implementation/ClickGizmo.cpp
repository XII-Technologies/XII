#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ClickGizmo.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiClickGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiClickGizmo::xiiClickGizmo()
{
  m_hShape.ConfigureHandle(this, xiiEngineGizmoHandleType::Sphere, xiiColor::White, xiiGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(xiiTransform::IdentityTransform());
}

void xiiClickGizmo::SetColor(const xiiColor& color)
{
  m_hShape.SetColor(color);
}

void xiiClickGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hShape);
}

void xiiClickGizmo::OnVisibleChanged(bool bVisible)
{
  m_hShape.SetVisible(bVisible);
}

void xiiClickGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  m_hShape.SetTransformation(transform);
}

void xiiClickGizmo::DoFocusLost(bool bCancel)
{
  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);
}

xiiEditorInput xiiClickGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle != &m_hShape)
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  SetActiveInputContext(this);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiClickGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}
