#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/BoxGizmo.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiBoxGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiBoxGizmo::xiiBoxGizmo()
{
  m_vSize.Set(1.0f);

  m_ManipulateMode = ManipulateMode::None;

  m_hCorners.ConfigureHandle(this, xiiEngineGizmoHandleType::BoxCorners, xiiColorLinearUB(200, 200, 200, 128), xiiGizmoFlags::Pickable);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].ConfigureHandle(this, xiiEngineGizmoHandleType::BoxEdges, xiiColorLinearUB(200, 200, 200, 128), xiiGizmoFlags::Pickable);
    m_Faces[i].ConfigureHandle(this, xiiEngineGizmoHandleType::BoxFaces, xiiColorLinearUB(200, 200, 200, 128), xiiGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(xiiTransform::IdentityTransform());
}

void xiiBoxGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hCorners);

  for (int i = 0; i < 3; ++i)
  {
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Edges[i]);
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Faces[i]);
  }
}

void xiiBoxGizmo::OnVisibleChanged(bool bVisible)
{
  m_hCorners.SetVisible(bVisible);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].SetVisible(bVisible);
    m_Faces[i].SetVisible(bVisible);
  }
}

void xiiBoxGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  xiiMat4 scale, rot;
  scale.SetScalingMatrix(m_vSize);
  scale = transform.GetAsMat4() * scale;

  m_hCorners.SetTransformation(scale);

  rot.SetRotationMatrixX(xiiAngle::Degree(90));
  m_Edges[0].SetTransformation(scale * rot);

  rot.SetRotationMatrixY(xiiAngle::Degree(90));
  m_Faces[0].SetTransformation(scale * rot);

  rot.SetIdentity();
  m_Edges[1].SetTransformation(scale * rot);

  rot.SetRotationMatrixX(xiiAngle::Degree(90));
  m_Faces[1].SetTransformation(scale * rot);

  rot.SetRotationMatrixZ(xiiAngle::Degree(90));
  m_Edges[2].SetTransformation(scale * rot);

  rot.SetIdentity();
  m_Faces[2].SetTransformation(scale * rot);
}

void xiiBoxGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
}

xiiEditorInput xiiBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hCorners)
  {
    m_ManipulateMode = ManipulateMode::Uniform;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[0])
  {
    m_ManipulateMode = ManipulateMode::AxisX;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[1])
  {
    m_ManipulateMode = ManipulateMode::AxisY;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[2])
  {
    m_ManipulateMode = ManipulateMode::AxisZ;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[0])
  {
    m_ManipulateMode = ManipulateMode::PlaneXY;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[1])
  {
    m_ManipulateMode = ManipulateMode::PlaneXZ;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[2])
  {
    m_ManipulateMode = ManipulateMode::PlaneYZ;
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

xiiEditorInput xiiBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastInteraction < xiiTime::Seconds(1.0 / 25.0))
    return xiiEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const xiiVec2I32 vNewMousePos = xiiVec2I32(mousePosition.x(), mousePosition.y());
  const xiiVec2I32 vDiff        = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed  = 0.02f;
  float       fChange = 0.0f;

  {
    fChange += vDiff.x * fSpeed;
    fChange -= vDiff.y * fSpeed;
  }

  xiiVec3 vChange(0);

  if (m_ManipulateMode == ManipulateMode::Uniform)
    vChange.Set(fChange);
  if (m_ManipulateMode == ManipulateMode::PlaneXY)
    vChange.Set(fChange, fChange, 0);
  if (m_ManipulateMode == ManipulateMode::PlaneXZ)
    vChange.Set(fChange, 0, fChange);
  if (m_ManipulateMode == ManipulateMode::PlaneYZ)
    vChange.Set(0, fChange, fChange);
  if (m_ManipulateMode == ManipulateMode::AxisX)
    vChange.Set(fChange, 0, 0);
  if (m_ManipulateMode == ManipulateMode::AxisY)
    vChange.Set(0, fChange, 0);
  if (m_ManipulateMode == ManipulateMode::AxisZ)
    vChange.Set(0, 0, fChange);

  m_vSize += vChange;
  m_vSize.x = xiiMath::Max(m_vSize.x, 0.0f);
  m_vSize.y = xiiMath::Max(m_vSize.y, 0.0f);
  m_vSize.z = xiiMath::Max(m_vSize.z, 0.0f);

  // update the scale
  OnTransformationChanged(GetTransformation());

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

void xiiBoxGizmo::SetSize(const xiiVec3& vSize)
{
  m_vSize = vSize;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
