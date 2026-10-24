/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScaleGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiScaleGizmo::xiiScaleGizmo()
{
  const xiiColor colr = xiiColorScheme::LightUI(xiiColorScheme::Red);
  const xiiColor colg = xiiColorScheme::LightUI(xiiColorScheme::Green);
  const xiiColor colb = xiiColorScheme::LightUI(xiiColorScheme::Blue);
  const xiiColor coly = xiiColorScheme::LightUI(xiiColorScheme::Gray);

  m_hAxisX.ConfigureHandle(this, xiiEngineGizmoHandleType::ScaleAxis, colr, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAxisY.ConfigureHandle(this, xiiEngineGizmoHandleType::ScaleAxis, colg, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAxisZ.ConfigureHandle(this, xiiEngineGizmoHandleType::ScaleAxis, colb, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAxisXYZ.ConfigureHandle(this, xiiEngineGizmoHandleType::ScaleUniform, coly, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);

  SetVisible(false);
  SetTransformation(xiiTransform::MakeIdentity());
}

void xiiScaleGizmo::UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow)
{
  const xiiVec3 scale(1.0f);
  GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Scale: {}, {}, {}", xiiArgF(scale.x, 2), xiiArgF(scale.y, 2), xiiArgF(scale.z, 2)));
}

void xiiScaleGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisXYZ);
}

void xiiScaleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible);
  m_hAxisY.SetVisible(bVisible);
  m_hAxisZ.SetVisible(bVisible);
  m_hAxisXYZ.SetVisible(bVisible);
}

void xiiScaleGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  auto axis        = transform;
  axis.m_qRotation = transform.m_qRotation * xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3::MakeAxisY());
  m_hAxisY.SetTransformation(axis);
  axis.m_qRotation = transform.m_qRotation * xiiQuat::MakeShortestRotation(xiiVec3::MakeAxisX(), xiiVec3::MakeAxisZ());
  m_hAxisZ.SetTransformation(axis);
  m_hAxisXYZ.SetTransformation(transform);
}

void xiiScaleGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hAxisX.SetVisible(true);
  m_hAxisY.SetVisible(true);
  m_hAxisZ.SetVisible(true);
  m_hAxisXYZ.SetVisible(true);
}

xiiEditorInput xiiScaleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vMoveAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vMoveAxis.Set(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vMoveAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisXYZ)
  {
    m_vMoveAxis.Set(1, 1, 1);
  }
  else
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  const auto pressPosition = e->globalPosition().toPoint();
  m_vLastMousePos.Set(pressPosition.x(), pressPosition.y());

  m_vScalingResult.Set(1.0f);
  m_vScaleMouseMove.SetZero();

  // Uniform scaling keeps the right/up gesture. Axis scaling follows the displayed
  // local axis, including cameras looking from the opposite side of the gizmo.
  m_vScreenAxis.Set(1, -1);
  if (m_pInteractionGizmoHandle != &m_hAxisXYZ)
  {
    xiiMat4 projection;
    m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, projection);
    const xiiMat4 viewProjection = projection * m_pCamera->GetViewMatrix();
    const auto&   transform      = GetTransformation();
    const float   distance       = xiiMath::Max(1.0f, (transform.m_vPosition - m_pCamera->GetCenterPosition()).GetLength()) * 0.01f;
    const xiiVec3 axis           = transform.m_qRotation * m_vMoveAxis;
    xiiVec3       start, end;
    if (xiiGraphicsUtils::ConvertWorldPosToScreenPos(viewProjection, 0, 0, m_vViewport.x, m_vViewport.y, transform.m_vPosition, start).Succeeded() &&
        xiiGraphicsUtils::ConvertWorldPosToScreenPos(viewProjection, 0, 0, m_vViewport.x, m_vViewport.y, transform.m_vPosition + axis * distance, end).Succeeded())
    {
      m_vScreenAxis.Set(end.x - start.x, end.y - start.y);
      m_vScreenAxis.NormalizeIfNotZero(xiiVec2(1, -1).GetNormalized()).IgnoreResult();
    }
  }

  SetActiveInputContext(this);

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiScaleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
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

xiiEditorInput xiiScaleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const xiiVec2I32 vNewMousePos = xiiVec2I32(mousePosition.x(), mousePosition.y());
  xiiVec2I32       vDiff        = (vNewMousePos - m_vLastMousePos);

  m_vLastMousePos = UpdateMouseMode(e);

  m_vScaleMouseMove += m_vMoveAxis * m_vScreenAxis.Dot(xiiVec2((float)vDiff.x, (float)vDiff.y));

  m_vScalingResult.Set(1.0f);

  const float fScaleSpeed = 0.01f;

  if (m_vScaleMouseMove.x > 0.0f)
    m_vScalingResult.x = 1.0f + m_vScaleMouseMove.x * fScaleSpeed;
  if (m_vScaleMouseMove.x < 0.0f)
    m_vScalingResult.x = 1.0f / (1.0f - m_vScaleMouseMove.x * fScaleSpeed);

  if (m_vScaleMouseMove.y > 0.0f)
    m_vScalingResult.y = 1.0f + m_vScaleMouseMove.y * fScaleSpeed;
  if (m_vScaleMouseMove.y < 0.0f)
    m_vScalingResult.y = 1.0f / (1.0f - m_vScaleMouseMove.y * fScaleSpeed);

  if (m_vScaleMouseMove.z > 0.0f)
    m_vScalingResult.z = 1.0f + m_vScaleMouseMove.z * fScaleSpeed;
  if (m_vScaleMouseMove.z < 0.0f)
    m_vScalingResult.z = 1.0f / (1.0f - m_vScaleMouseMove.z * fScaleSpeed);

  // disable snapping when SHIFT is pressed
  if (!e->modifiers().testFlag(Qt::ShiftModifier))
    xiiSnapProvider::SnapScale(m_vScalingResult);

  GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Scale: {}, {}, {}", xiiArgF(m_vScalingResult.x, 2), xiiArgF(m_vScalingResult.y, 2), xiiArgF(m_vScalingResult.z, 2)));
  xiiStringBuilder feedback;
  feedback.SetFormat("Scale   X {}x  Y {}x  Z {}x", xiiArgF(m_vScalingResult.x, 2), xiiArgF(m_vScalingResult.y, 2), xiiArgF(m_vScalingResult.z, 2));
  static_cast<xiiEngineGizmoHandle*>(m_pInteractionGizmoHandle)->SetInteractionText(feedback);

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiManipulatorScaleGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiManipulatorScaleGizmo::xiiManipulatorScaleGizmo()
{
  const xiiColor colr = xiiColorScheme::LightUI(xiiColorScheme::Red);
  const xiiColor colg = xiiColorScheme::LightUI(xiiColorScheme::Green);
  const xiiColor colb = xiiColorScheme::LightUI(xiiColorScheme::Blue);

  // Overwrite axis to be boxes.
  m_hAxisX.ConfigureHandle(this, xiiEngineGizmoHandleType::Box, colr, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAxisY.ConfigureHandle(this, xiiEngineGizmoHandleType::Box, colg, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAxisZ.ConfigureHandle(this, xiiEngineGizmoHandleType::Box, colb, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
  m_hAxisXYZ.ConfigureHandle(this, xiiEngineGizmoHandleType::Box, xiiColorScheme::LightUI(xiiColorScheme::Yellow), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho);
}

void xiiManipulatorScaleGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  const float  fOffset = 0.8f;
  xiiTransform t;
  t.SetIdentity();
  t.m_vPosition = xiiVec3(fOffset, 0, 0);
  t.m_vScale    = xiiVec3(0.2f);

  m_hAxisX.SetTransformation(transform * t);

  t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::MakeFromDegree(90));
  t.m_vPosition = xiiVec3(0, fOffset, 0);
  m_hAxisY.SetTransformation(transform * t);

  t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(-90));
  t.m_vPosition = xiiVec3(0, 0, fOffset);
  m_hAxisZ.SetTransformation(transform * t);

  t.SetIdentity();
  t.m_vScale = xiiVec3(0.3f);
  m_hAxisXYZ.SetTransformation(transform * t);
}
