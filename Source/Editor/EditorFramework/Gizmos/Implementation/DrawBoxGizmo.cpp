#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDrawBoxGizmo, 1, xiiRTTINoAllocator);
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDrawBoxGizmo::xiiDrawBoxGizmo()
{
  m_ManipulateMode = ManipulateMode::None;

  m_vLastStartPoint.SetZero();
  m_hBox.ConfigureHandle(this, xiiEngineGizmoHandleType::LineBox, xiiColorLinearUB(255, 100, 0), xiiGizmoFlags::ShowInOrtho);

  SetVisible(false);
  SetTransformation(xiiTransform::MakeIdentity());
}

xiiDrawBoxGizmo::~xiiDrawBoxGizmo() = default;

void xiiDrawBoxGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hBox);
}

void xiiDrawBoxGizmo::OnVisibleChanged(bool bVisible) {}

void xiiDrawBoxGizmo::OnTransformationChanged(const xiiTransform& transform) {}

void xiiDrawBoxGizmo::DoFocusLost(bool bCancel)
{
  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
  UpdateBox();

  if (IsActiveInputContext())
    SetActiveInputContext(nullptr);
}

bool xiiDrawBoxGizmo::PickPosition(QMouseEvent* e)
{
  const QPoint mousePos = GetOwnerWindow()->mapFromGlobal(QCursor::pos());

  const xiiObjectPickingResult& res = GetOwnerView()->PickObject(mousePos.x(), mousePos.y());

  m_vUpAxis   = GetOwnerView()->GetFallbackPickingPlane().m_vNormal;
  m_vUpAxis.x = xiiMath::Abs(m_vUpAxis.x);
  m_vUpAxis.y = xiiMath::Abs(m_vUpAxis.y);
  m_vUpAxis.z = xiiMath::Abs(m_vUpAxis.z);

  if (res.m_PickedObject.IsValid() && !e->modifiers().testFlag(Qt::ShiftModifier))
  {
    m_vCurrentPosition = res.m_vPickedPosition;
  }
  else
  {
    if (GetOwnerView()->PickPlane(e->pos().x(), e->pos().y(), GetOwnerView()->GetFallbackPickingPlane(m_vLastStartPoint), m_vCurrentPosition).Failed())
    {
      return false;
    }
  }

  xiiSnapProvider::SnapTranslation(m_vCurrentPosition);
  return true;
}

xiiEditorInput xiiDrawBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (e->buttons() == Qt::LeftButton && e->modifiers().testFlag(Qt::ControlModifier))
  {
    if (m_ManipulateMode == ManipulateMode::None)
    {
      if (!PickPosition(e))
      {
        return xiiEditorInput::WasExclusivelyHandled; // failed to pick anything
      }

      m_vLastStartPoint = m_vCurrentPosition;
      SwitchMode(false);
      return xiiEditorInput::WasExclusivelyHandled;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiDrawBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() == Qt::LeftButton)
  {
    if (m_ManipulateMode == ManipulateMode::DrawBase || m_ManipulateMode == ManipulateMode::DrawHeight)
    {
      SwitchMode(m_vFirstCorner == m_vSecondCorner);
      return xiiEditorInput::WasExclusivelyHandled;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiDrawBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  UpdateGrid(e);

  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    const QPoint     mousePosition = e->globalPosition().toPoint();
    const xiiVec2I32 vMouseMove    = xiiVec2I32(mousePosition.x(), mousePosition.y()) - m_vLastMousePos;
    m_iHeightChange -= vMouseMove.y;

    m_vLastMousePos = UpdateMouseMode(e);
  }
  else
  {
    xiiPlane plane;
    plane = xiiPlane::MakeFromNormalAndPoint(m_vUpAxis, m_vFirstCorner);

    GetOwnerView()->PickPlane(e->pos().x(), e->pos().y(), plane, m_vCurrentPosition).IgnoreResult();

    xiiSnapProvider::SnapTranslation(m_vCurrentPosition);
  }

  UpdateBox();

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiDrawBoxGizmo::DoKeyPressEvent(QKeyEvent* e)
{
  // is the gizmo in general visible == is it active
  if (!IsVisible())
    return xiiEditorInput::MayBeHandledByOthers;

  DisableGrid(e->modifiers().testFlag(Qt::ControlModifier));

  if (e->key() == Qt::Key_Escape)
  {
    if (m_ManipulateMode != ManipulateMode::None)
    {
      SwitchMode(true);
      return xiiEditorInput::WasExclusivelyHandled;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiDrawBoxGizmo::DoKeyReleaseEvent(QKeyEvent* e)
{
  DisableGrid(e->modifiers().testFlag(Qt::ControlModifier));

  return xiiEditorInput::MayBeHandledByOthers;
}

void xiiDrawBoxGizmo::SwitchMode(bool bCancel)
{
  xiiGizmoEvent e;
  e.m_pGizmo = this;

  if (bCancel)
  {
    FocusLost(true);

    e.m_Type = xiiGizmoEvent::Type::CancelInteractions;
    m_GizmoEvents.Broadcast(e);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::None)
  {
    m_ManipulateMode = ManipulateMode::DrawBase;
    m_vFirstCorner   = m_vCurrentPosition;
    m_vSecondCorner  = m_vFirstCorner;

    SetActiveInputContext(this);
    UpdateBox();

    e.m_Type = xiiGizmoEvent::Type::BeginInteractions;
    m_GizmoEvents.Broadcast(e);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawBase)
  {
    m_ManipulateMode     = ManipulateMode::DrawHeight;
    m_iHeightChange      = 0;
    m_fOriginalBoxHeight = m_fBoxHeight;
    m_vLastMousePos      = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
    UpdateBox();
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    e.m_Type = xiiGizmoEvent::Type::EndInteractions;
    m_GizmoEvents.Broadcast(e);

    UpdateBox();
    FocusLost(false);
    return;
  }
}

void xiiDrawBoxGizmo::UpdateBox()
{
  UpdateStatusBarText(GetOwnerWindow());

  if (m_ManipulateMode == ManipulateMode::DrawBase)
  {
    m_vSecondCorner   = m_vCurrentPosition;
    m_vSecondCorner.x = xiiMath::Lerp(m_vSecondCorner.x, m_vFirstCorner.x, m_vUpAxis.x);
    m_vSecondCorner.y = xiiMath::Lerp(m_vSecondCorner.y, m_vFirstCorner.y, m_vUpAxis.y);
    m_vSecondCorner.z = xiiMath::Lerp(m_vSecondCorner.z, m_vFirstCorner.z, m_vUpAxis.z);
  }

  if (m_ManipulateMode == ManipulateMode::None || m_vFirstCorner == m_vSecondCorner)
  {
    m_hBox.SetTransformation(xiiTransform(xiiVec3(0), xiiQuat::MakeIdentity(), xiiVec3(0)));
    m_hBox.SetVisible(false);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    m_fBoxHeight = m_fOriginalBoxHeight + ((float)m_iHeightChange * 0.1f * xiiSnapProvider::GetTranslationSnapValue());
    xiiVec3 snapDummy(m_fBoxHeight);
    xiiSnapProvider::SnapTranslation(snapDummy);
    m_fBoxHeight = m_vUpAxis.Dot(snapDummy);
  }

  xiiVec3 vCenter = xiiMath::Lerp(m_vFirstCorner, m_vSecondCorner, 0.5f);
  vCenter.x += m_fBoxHeight * 0.5f * m_vUpAxis.x;
  vCenter.y += m_fBoxHeight * 0.5f * m_vUpAxis.y;
  vCenter.z += m_fBoxHeight * 0.5f * m_vUpAxis.z;

  xiiVec3 vSize;

  if (m_vUpAxis.z != 0)
  {
    vSize.x = xiiMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
    vSize.y = xiiMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
    vSize.z = m_fBoxHeight;
  }
  else if (m_vUpAxis.x != 0)
  {
    vSize.z = xiiMath::Abs(m_vSecondCorner.z - m_vFirstCorner.z);
    vSize.y = xiiMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
    vSize.x = m_fBoxHeight;
  }
  else if (m_vUpAxis.y != 0)
  {
    vSize.x = xiiMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
    vSize.z = xiiMath::Abs(m_vSecondCorner.z - m_vFirstCorner.z);
    vSize.y = m_fBoxHeight;
  }

  m_hBox.SetTransformation(xiiTransform(vCenter, xiiQuat::MakeIdentity(), vSize));
  m_hBox.SetVisible(true);
}

void xiiDrawBoxGizmo::DisableGrid(bool bControlPressed)
{
  if (!bControlPressed)
  {
    m_bDisplayGrid = false;
  }
}

void xiiDrawBoxGizmo::UpdateGrid(QMouseEvent* e)
{
  m_bDisplayGrid = false;

  if (m_ManipulateMode == ManipulateMode::None && e->modifiers().testFlag(Qt::ControlModifier))
  {
    if (e != nullptr && PickPosition(e))
    {
      m_vFirstCorner = m_vCurrentPosition;
      m_bDisplayGrid = true;
    }
  }
}

void xiiDrawBoxGizmo::GetResult(xiiVec3& out_vOrigin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY, float& out_fSizeNegZ, float& out_fSizePosZ) const
{
  out_vOrigin = m_vFirstCorner;

  float fBoxX = m_vSecondCorner.x - m_vFirstCorner.x;
  float fBoxY = m_vSecondCorner.y - m_vFirstCorner.y;
  float fBoxZ = m_fBoxHeight;

  if (m_vUpAxis.x != 0)
  {
    fBoxY = m_vSecondCorner.y - m_vFirstCorner.y;
    fBoxZ = m_vSecondCorner.z - m_vFirstCorner.z;
    fBoxX = m_fBoxHeight;
  }

  if (m_vUpAxis.y != 0)
  {
    fBoxX = m_vSecondCorner.x - m_vFirstCorner.x;
    fBoxZ = m_vSecondCorner.z - m_vFirstCorner.z;
    fBoxY = m_fBoxHeight;
  }

  if (fBoxX > 0)
  {
    out_fSizeNegX = 0;
    out_fSizePosX = fBoxX;
  }
  else
  {
    out_fSizeNegX = -fBoxX;
    out_fSizePosX = 0;
  }

  if (fBoxY > 0)
  {
    out_fSizeNegY = 0;
    out_fSizePosY = fBoxY;
  }
  else
  {
    out_fSizeNegY = -fBoxY;
    out_fSizePosY = 0;
  }

  if (fBoxZ > 0)
  {
    out_fSizeNegZ = 0;
    out_fSizePosZ = fBoxZ;
  }
  else
  {
    out_fSizeNegZ = -fBoxZ;
    out_fSizePosZ = 0;
  }
}

void xiiDrawBoxGizmo::UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow)
{
  switch (m_ManipulateMode)
  {
    case ManipulateMode::None:
    {
      pWindow->SetPermanentStatusBarMsg("Greyboxing: Hold CTRL and click-drag to draw a box. Hold SHIFT to reuse the previous plane height.");
      break;
    }

    case ManipulateMode::DrawBase:
    {
      xiiVec3 diff = m_vSecondCorner - m_vFirstCorner;
      diff.x       = xiiMath::Abs(diff.x);
      diff.y       = xiiMath::Abs(diff.y);

      pWindow->SetPermanentStatusBarMsg(xiiFmt("Greyboxing: [Width: {}, Depth: {}, Height: {}] Release the mouse to finish the base. ESC to cancel.", xiiArgF(diff.y, 2, false, 2), xiiArgF(diff.x, 2, false, 2), xiiArgF(m_fBoxHeight, 2, false, 2)));
      break;
    }

    case ManipulateMode::DrawHeight:
    {
      xiiVec3 diff = m_vSecondCorner - m_vFirstCorner;
      diff.x       = xiiMath::Abs(diff.x);
      diff.y       = xiiMath::Abs(diff.y);

      pWindow->SetPermanentStatusBarMsg(xiiFmt("Greyboxing: [Width: {}, Depth: {}, Height: {}] Draw up/down to specify the box height. Click to finish, ESC to cancel.", xiiArgF(diff.y, 2, false, 2), xiiArgF(diff.x, 2, false, 2), xiiArgF(m_fBoxHeight, 2, false, 2)));
      break;
    }
  }
}
