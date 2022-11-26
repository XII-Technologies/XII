#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCapsuleGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiCapsuleGizmo::xiiCapsuleGizmo()
{
  m_fLength = 1.0f;
  m_fRadius = 0.25f;

  m_ManipulateMode = ManipulateMode::None;

  m_hRadius.ConfigureHandle(this, xiiEngineGizmoHandleType::CylinderZ, xiiColorLinearUB(200, 200, 200, 128), xiiGizmoFlags::Pickable);
  m_hLengthTop.ConfigureHandle(this, xiiEngineGizmoHandleType::HalfSphereZ, xiiColorLinearUB(200, 200, 200, 128), xiiGizmoFlags::Pickable);
  m_hLengthBottom.ConfigureHandle(this, xiiEngineGizmoHandleType::HalfSphereZ, xiiColorLinearUB(200, 200, 200, 128), xiiGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(xiiTransform::IdentityTransform());
}

void xiiCapsuleGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hLengthTop);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hLengthBottom);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hRadius);
}

void xiiCapsuleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hLengthTop.SetVisible(bVisible);
  m_hLengthBottom.SetVisible(bVisible);
  m_hRadius.SetVisible(bVisible);
}

void xiiCapsuleGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  {
    xiiTransform mScaleCylinder;
    mScaleCylinder.SetIdentity();
    mScaleCylinder.m_vScale = xiiVec3(m_fRadius, m_fRadius, m_fLength);

    m_hRadius.SetTransformation(transform * mScaleCylinder);
  }

  {
    xiiTransform mScaleSpheres;
    mScaleSpheres.SetIdentity();
    mScaleSpheres.m_vScale.Set(m_fRadius);
    mScaleSpheres.m_vPosition.Set(0, 0, m_fLength * 0.5f);
    m_hLengthTop.SetTransformation(transform * mScaleSpheres);
  }

  {
    xiiTransform mScaleSpheres;
    mScaleSpheres.SetIdentity();
    mScaleSpheres.m_vScale.Set(m_fRadius, -m_fRadius, -m_fRadius);
    mScaleSpheres.m_vPosition.Set(0, 0, -m_fLength * 0.5f);
    m_hLengthBottom.SetTransformation(transform * mScaleSpheres);
  }
}

void xiiCapsuleGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hLengthTop.SetVisible(true);
  m_hLengthBottom.SetVisible(true);
  m_hRadius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

xiiEditorInput xiiCapsuleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
  }
  else if (m_pInteractionGizmoHandle == &m_hLengthTop || m_pInteractionGizmoHandle == &m_hLengthBottom)
  {
    m_ManipulateMode = ManipulateMode::Length;
  }
  else
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = xiiTime::Now();

  m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiCapsuleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiCapsuleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastInteraction < xiiTime::Seconds(1.0 / 25.0))
    return xiiEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const xiiVec2I32 vNewMousePos = xiiVec2I32(e->globalPos().x(), e->globalPos().y());
  const xiiVec2I32 vDiff        = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = xiiMath::Max(0.0f, m_fRadius);
  }
  else
  {
    m_fLength += vDiff.x * fSpeed;
    m_fLength -= vDiff.y * fSpeed;

    m_fLength = xiiMath::Max(0.0f, m_fLength);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

void xiiCapsuleGizmo::SetLength(float fRadius)
{
  m_fLength = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

void xiiCapsuleGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
