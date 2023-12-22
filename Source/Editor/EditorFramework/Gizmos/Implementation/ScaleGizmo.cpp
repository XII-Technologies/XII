#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScaleGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiScaleGizmo::xiiScaleGizmo()
{
  const xiiColor colr = xiiColorScheme::LightUI(xiiColorScheme::Red);
  const xiiColor colg = xiiColorScheme::LightUI(xiiColorScheme::Green);
  const xiiColor colb = xiiColorScheme::LightUI(xiiColorScheme::Blue);
  const xiiColor coly = xiiColorScheme::LightUI(xiiColorScheme::Gray);

  m_hAxisX.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colr, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowX.obj");
  m_hAxisY.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colg, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowY.obj");
  m_hAxisZ.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colb, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowZ.obj");
  m_hAxisXYZ.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, coly, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/ScaleXYZ.obj");

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
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
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

  m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_vScalingResult.Set(1.0f);
  m_vScaleMouseMove.SetZero();

  xiiMat4 mView = m_pCamera->GetViewMatrix();
  xiiMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  xiiMat4 mViewProj = mProj * mView;
  m_mInvViewProj    = mViewProj.GetInverse();

  m_LastInteraction = xiiTime::Now();

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

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiScaleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastInteraction < xiiTime::MakeFromSeconds(1.0 / 25.0))
    return xiiEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const xiiVec2I32 vNewMousePos = xiiVec2I32(mousePosition.x(), mousePosition.y());
  xiiVec2I32       vDiff        = (vNewMousePos - m_vLastMousePos);

  m_vLastMousePos = UpdateMouseMode(e);

  m_vScaleMouseMove += m_vMoveAxis * (float)vDiff.x;
  m_vScaleMouseMove -= m_vMoveAxis * (float)vDiff.y;

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

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    xiiSnapProvider::SnapScale(m_vScalingResult);

  GetOwnerWindow()->SetPermanentStatusBarMsg(
    xiiFmt("Scale: {}, {}, {}", xiiArgF(m_vScalingResult.x, 2), xiiArgF(m_vScalingResult.y, 2), xiiArgF(m_vScalingResult.z, 2)));

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
  // Overwrite axis to be boxes.
  m_hAxisX.ConfigureHandle(this, xiiEngineGizmoHandleType::Box, xiiColorLinearUB(128, 0, 0), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable);
  m_hAxisY.ConfigureHandle(this, xiiEngineGizmoHandleType::Box, xiiColorLinearUB(0, 128, 0), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable);
  m_hAxisZ.ConfigureHandle(this, xiiEngineGizmoHandleType::Box, xiiColorLinearUB(0, 0, 128), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable);
}

void xiiManipulatorScaleGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  const float  fOffset = 0.8f;
  xiiTransform t;
  t.SetIdentity();
  t.m_vPosition = xiiVec3(fOffset, 0, 0);
  t.m_vScale    = xiiVec3(0.2f);

  m_hAxisX.SetTransformation(transform * t);

  t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 0, 1), xiiAngle::Degree(90));
  t.m_vPosition = xiiVec3(0, fOffset, 0);
  m_hAxisY.SetTransformation(transform * t);

  t.m_qRotation = xiiQuat::MakeFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(-90));
  t.m_vPosition = xiiVec3(0, 0, fOffset);
  m_hAxisZ.SetTransformation(transform * t);

  t.SetIdentity();
  t.m_vScale = xiiVec3(0.3f);
  m_hAxisXYZ.SetTransformation(transform * t);
}
