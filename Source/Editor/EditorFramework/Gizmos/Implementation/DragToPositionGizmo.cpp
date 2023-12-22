#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDragToPositionGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDragToPositionGizmo::xiiDragToPositionGizmo()
{
  m_bModifiesRotation = false;

  // TODO: adjust colors for +/- axis
  const xiiColor colr1 = xiiColorGammaUB(206, 0, 46);
  const xiiColor colr2 = xiiColorGammaUB(206, 0, 46);
  const xiiColor colg1 = xiiColorGammaUB(101, 206, 0);
  const xiiColor colg2 = xiiColorGammaUB(101, 206, 0);
  const xiiColor colb1 = xiiColorGammaUB(0, 125, 206);
  const xiiColor colb2 = xiiColorGammaUB(0, 125, 206);
  const xiiColor coly  = xiiColorGammaUB(128, 128, 0);

  m_hBobble.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, coly, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/DragCenter.obj");
  m_hAlignPX.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colr1, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/DragArrowPX.obj");
  m_hAlignNX.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colr2, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/DragArrowNX.obj");
  m_hAlignPY.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colg1, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/DragArrowPY.obj");
  m_hAlignNY.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colg2, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/DragArrowNY.obj");
  m_hAlignPZ.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colb1, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/DragArrowPZ.obj");
  m_hAlignNZ.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colb2, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/DragArrowNZ.obj");

  SetVisible(false);
  SetTransformation(xiiTransform::MakeIdentity());
}

void xiiDragToPositionGizmo::UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow)
{
  if (m_pInteractionGizmoHandle != nullptr)
  {
    if (m_pInteractionGizmoHandle == &m_hBobble)
      GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Drag to Position: Center"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Drag to Position: +X"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Drag to Position: -X"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Drag to Position: +Y"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Drag to Position: -Y"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Drag to Position: +Z"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Drag to Position: -Z"));
  }
  else
  {
    GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Drag to Position"));
  }
}

void xiiDragToPositionGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hBobble);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNZ);
}

void xiiDragToPositionGizmo::OnVisibleChanged(bool bVisible)
{
  m_hBobble.SetVisible(bVisible);
  m_hAlignPX.SetVisible(bVisible);
  m_hAlignNX.SetVisible(bVisible);
  m_hAlignPY.SetVisible(bVisible);
  m_hAlignNY.SetVisible(bVisible);
  m_hAlignPZ.SetVisible(bVisible);
  m_hAlignNZ.SetVisible(bVisible);
}

void xiiDragToPositionGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  m_hBobble.SetTransformation(transform);
  m_hAlignPX.SetTransformation(transform);
  m_hAlignNX.SetTransformation(transform);
  m_hAlignPY.SetTransformation(transform);
  m_hAlignNY.SetTransformation(transform);
  m_hAlignPZ.SetTransformation(transform);
  m_hAlignNZ.SetTransformation(transform);
}

void xiiDragToPositionGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hBobble.SetVisible(true);
  m_hAlignPX.SetVisible(true);
  m_hAlignNX.SetVisible(true);
  m_hAlignPY.SetVisible(true);
  m_hAlignNY.SetVisible(true);
  m_hAlignPZ.SetVisible(true);
  m_hAlignNZ.SetVisible(true);

  m_pInteractionGizmoHandle = nullptr;
}

xiiEditorInput xiiDragToPositionGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  // The gizmo is actually "hidden" somewhere else during dragging,
  // because it musn't be rendered into the picking buffer, to avoid picking against the gizmo
  // m_Bobble.SetVisible(false);
  // m_AlignPX.SetVisible(false);
  // m_AlignNX.SetVisible(false);
  // m_AlignPY.SetVisible(false);
  // m_AlignNY.SetVisible(false);
  // m_AlignPZ.SetVisible(false);
  // m_AlignNZ.SetVisible(false);
  // m_pInteractionGizmoHandle->SetVisible(true);

  m_vStartPosition    = GetTransformation().m_vPosition;
  m_qStartOrientation = GetTransformation().m_qRotation;

  m_LastInteraction = xiiTime::Now();

  SetActiveInputContext(this);

  UpdateStatusBarText(nullptr);

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiDragToPositionGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiDragToPositionGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastInteraction < xiiTime::Seconds(1.0 / 25.0))
    return xiiEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const xiiObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

  if (!res.m_PickedObject.IsValid())
    return xiiEditorInput::WasExclusivelyHandled;

  if (res.m_vPickedPosition.IsNaN() || res.m_vPickedNormal.IsNaN() || res.m_vPickedNormal.IsZero())
    return xiiEditorInput::WasExclusivelyHandled;

  xiiVec3 vSnappedPosition = res.m_vPickedPosition;

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    xiiSnapProvider::SnapTranslation(vSnappedPosition);

  xiiTransform mTrans = GetTransformation();
  mTrans.m_vPosition  = vSnappedPosition;

  xiiQuat rot;
  xiiVec3 alignAxis, orthoAxis;

  if (m_pInteractionGizmoHandle == &m_hAlignPX)
  {
    alignAxis.Set(1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNX)
  {
    alignAxis.Set(-1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignPY)
  {
    alignAxis.Set(0, 1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNY)
  {
    alignAxis.Set(0, -1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignPZ)
  {
    alignAxis.Set(0, 0, 1);
    orthoAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNZ)
  {
    alignAxis.Set(0, 0, -1);
    orthoAxis.Set(1, 0, 0);
  }
  else
  {
    m_bModifiesRotation = false;
    rot.SetIdentity();
  }

  if (m_pInteractionGizmoHandle != &m_hBobble)
  {
    m_bModifiesRotation = true;

    alignAxis = m_qStartOrientation * alignAxis;
    alignAxis.Normalize();

    if (alignAxis.GetAngleBetween(res.m_vPickedNormal) > xiiAngle::Degree(179))
    {
      rot = xiiQuat::MakeFromAxisAndAngle(m_qStartOrientation * orthoAxis, xiiAngle::Degree(180));
    }
    else
    {
      rot = xiiQuat::MakeShortestRotation(alignAxis, res.m_vPickedNormal);
    }
  }

  mTrans.m_qRotation = rot * m_qStartOrientation;
  SetTransformation(mTrans);

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}
