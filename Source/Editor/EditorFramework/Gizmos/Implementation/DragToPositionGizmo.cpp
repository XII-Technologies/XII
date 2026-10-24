/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

  m_hBobble.ConfigureHandle(this, xiiEngineGizmoHandleType::ScaleUniform, coly, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAlignPX.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colr1, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAlignNX.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colr2, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAlignPY.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colg1, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAlignNY.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colg2, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAlignPZ.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colb1, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAlignNZ.ConfigureHandle(this, xiiEngineGizmoHandleType::TranslationAxis, colb2, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);

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
  auto oriented = [&](const xiiVec3& direction) {
    auto result        = transform;
    result.m_qRotation = transform.m_qRotation * xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), direction);
    return result;
  };
  m_hAlignNX.SetTransformation(oriented(-xiiVec3::MakeAxisX()));
  m_hAlignPY.SetTransformation(oriented(xiiVec3::MakeAxisY()));
  m_hAlignNY.SetTransformation(oriented(-xiiVec3::MakeAxisY()));
  m_hAlignPZ.SetTransformation(oriented(xiiVec3::MakeAxisZ()));
  m_hAlignNZ.SetTransformation(oriented(-xiiVec3::MakeAxisZ()));
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

  if (m_pInteractionGizmoHandle == nullptr)
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vStartPosition    = GetTransformation().m_vPosition;
  m_qStartOrientation = GetTransformation().m_qRotation;

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

  // Commit the release position even if Qt coalesced the final mouse move.
  DoMouseMoveEvent(e);

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiDragToPositionGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  // Movement uses asynchronous surface queries; release must consume its own position.
  const xiiObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y(), e->type() == QEvent::MouseButtonRelease);

  if (!res.m_PickedObject.IsValid())
    return xiiEditorInput::WasExclusivelyHandled;

  if (!res.m_vPickedPosition.IsValid() || !res.m_vPickedNormal.IsValid() || res.m_vPickedNormal.IsZero())
    return xiiEditorInput::WasExclusivelyHandled;

  xiiVec3 vSnappedPosition = res.m_vPickedPosition;

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
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

    if (alignAxis.GetAngleBetween(res.m_vPickedNormal) > xiiAngle::MakeFromDegree(179))
    {
      rot = xiiQuat::MakeFromAxisAndAngle(m_qStartOrientation * orthoAxis, xiiAngle::MakeFromDegree(180));
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
