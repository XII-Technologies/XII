#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRotateGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRotateGizmo::xiiRotateGizmo()
{
  xiiEditorPreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiEditorPreferencesUser>();
  m_bUseExperimentalGizmo                = !pPreferences->m_bOldGizmos;

  if (m_bUseExperimentalGizmo)
  {
    const xiiColor colr = xiiColorScheme::LightUI(xiiColorScheme::Red);
    const xiiColor colg = xiiColorScheme::LightUI(xiiColorScheme::Green);
    const xiiColor colb = xiiColorScheme::LightUI(xiiColorScheme::Blue);

    m_hAxisX.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colr, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneX.obj");
    m_hAxisY.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colg, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneY.obj");
    m_hAxisZ.ConfigureHandle(this, xiiEngineGizmoHandleType::FromFile, colb, xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneZ.obj");
  }
  else
  {
    m_hAxisX.ConfigureHandle(this, xiiEngineGizmoHandleType::Ring, xiiColorLinearUB(128, 0, 0), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable);
    m_hAxisY.ConfigureHandle(this, xiiEngineGizmoHandleType::Ring, xiiColorLinearUB(0, 128, 0), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable);
    m_hAxisZ.ConfigureHandle(this, xiiEngineGizmoHandleType::Ring, xiiColorLinearUB(0, 0, 128), xiiGizmoFlags::ConstantSize | xiiGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(xiiTransform::IdentityTransform());
}

void xiiRotateGizmo::UpdateStatusBarText(xiiQtEngineDocumentWindow* pWindow)
{
  GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Rotation: {}", xiiAngle()));
}

void xiiRotateGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);
}

void xiiRotateGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible);
  m_hAxisY.SetVisible(bVisible);
  m_hAxisZ.SetVisible(bVisible);
}

void xiiRotateGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  if (m_bUseExperimentalGizmo)
  {
    m_hAxisX.SetTransformation(transform);
    m_hAxisY.SetTransformation(transform);
    m_hAxisZ.SetTransformation(transform);
  }
  else
  {
    xiiTransform m;
    m.SetIdentity();

    m.m_qRotation.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::Degree(-90));
    m_hAxisX.SetTransformation(transform * m);

    m.m_qRotation.SetFromAxisAndAngle(xiiVec3(1, 0, 0), xiiAngle::Degree(90));
    m_hAxisY.SetTransformation(transform * m);

    m.SetIdentity();
    m_hAxisZ.SetTransformation(transform * m);
  }
}

void xiiRotateGizmo::DoFocusLost(bool bCancel)
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
}

xiiEditorInput xiiRotateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiQuat gizmoRot = GetTransformation().m_qRotation;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vRotationAxis = gizmoRot * xiiVec3(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vRotationAxis = gizmoRot * xiiVec3(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vRotationAxis = gizmoRot * xiiVec3(0, 0, 1);
  }
  else
    return xiiEditorInput::MayBeHandledByOthers;

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_Rotation = xiiAngle();

  m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_qStartRotation = GetTransformation().m_qRotation;

  xiiMat4 mView = m_pCamera->GetViewMatrix();
  xiiMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  xiiMat4 mViewProj = mProj * mView;
  m_mInvViewProj    = mViewProj.GetInverse();

  // compute screen space tangent for rotation
  {
    const xiiVec3 vAxisWS = m_vRotationAxis.GetNormalized();
    const xiiVec3 vMousePos(e->pos().x(), m_vViewport.y - e->pos().y(), 0);
    const xiiVec3 vGizmoPosWS = GetTransformation().m_vPosition;

    xiiVec3 vPosOnNearPlane, vRayDir;
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vMousePos, vPosOnNearPlane, &vRayDir).IgnoreResult();

    xiiPlane plane;
    plane.SetFromNormalAndPoint(vAxisWS, vGizmoPosWS);

    xiiVec3 vPointOnGizmoWS;
    if (!plane.GetRayIntersection(vPosOnNearPlane, vRayDir, nullptr, &vPointOnGizmoWS))
    {
      // fallback at grazing angles, will result in fallback vDirWS during normalization
      vPointOnGizmoWS = vGizmoPosWS;
    }

    xiiVec3 vDirWS = vPointOnGizmoWS - vGizmoPosWS;
    vDirWS.NormalizeIfNotZero(xiiVec3(1, 0, 0)).IgnoreResult();

    xiiVec3 vTangentWS = vAxisWS.CrossRH(vDirWS);
    vTangentWS.Normalize();

    const xiiVec3 vTangentEndWS = vPointOnGizmoWS + vTangentWS;

    // compute the screen space position of the end point of the tangent vector, so that we can then compute the tangent in screen space
    xiiVec3 vTangentEndSS;
    xiiGraphicsUtils::ConvertWorldPosToScreenPos(mViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vTangentEndWS, vTangentEndSS).IgnoreResult();
    vTangentEndSS.z = 0;

    const xiiVec3 vTangentSS = vTangentEndSS - vMousePos;
    m_vScreenTangent.Set(vTangentSS.x, vTangentSS.y);
    m_vScreenTangent.NormalizeIfNotZero(xiiVec2(1, 0)).IgnoreResult();

    // because window coordinates are flipped along Y
    m_vScreenTangent.y = -m_vScreenTangent.y;
  }

  m_LastInteraction = xiiTime::Now();

  SetActiveInputContext(this);

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiRotateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiRotateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastInteraction < xiiTime::Seconds(1.0 / 25.0))
    return xiiEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const xiiVec2 vNewMousePos = xiiVec2(e->globalPos().x(), e->globalPos().y());
  xiiVec2       vDiff        = vNewMousePos - xiiVec2(m_vLastMousePos.x, m_vLastMousePos.y);

  m_vLastMousePos = UpdateMouseMode(e);

  const float dv = m_vScreenTangent.Dot(vDiff);
  m_Rotation += xiiAngle::Degree(dv);

  xiiAngle rot = m_Rotation;

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    xiiSnapProvider::SnapRotation(rot);

  m_qCurrentRotation.SetFromAxisAndAngle(m_vRotationAxis, rot);

  xiiTransform mTrans = GetTransformation();
  mTrans.m_qRotation  = m_qCurrentRotation * m_qStartRotation;

  SetTransformation(mTrans);

  GetOwnerWindow()->SetPermanentStatusBarMsg(xiiFmt("Rotation: {}", rot));

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}
