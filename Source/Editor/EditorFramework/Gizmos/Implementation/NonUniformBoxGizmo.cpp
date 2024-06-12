#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <Foundation/Utilities/GraphicsUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiNonUniformBoxGizmo, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiNonUniformBoxGizmo::xiiNonUniformBoxGizmo()
{
  m_vNegSize.Set(1.0f);
  m_vPosSize.Set(1.0f);

  m_ManipulateMode = ManipulateMode::None;

  m_hOutline.ConfigureHandle(this, xiiEngineGizmoHandleType::LineBox, xiiColorScheme::LightUI(xiiColorScheme::Gray), xiiGizmoFlags::ShowInOrtho);

  xiiColor cols[6] = {
    xiiColorScheme::LightUI(xiiColorScheme::Red),
    xiiColorScheme::LightUI(xiiColorScheme::Red),
    xiiColorScheme::LightUI(xiiColorScheme::Green),
    xiiColorScheme::LightUI(xiiColorScheme::Green),
    xiiColorScheme::LightUI(xiiColorScheme::Blue),
    xiiColorScheme::LightUI(xiiColorScheme::Blue),
  };

  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    m_Nobs[i].ConfigureHandle(this, xiiEngineGizmoHandleType::Box, cols[i], xiiGizmoFlags::ConstantSize | xiiGizmoFlags::OnTop | xiiGizmoFlags::ShowInOrtho | xiiGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(xiiTransform::MakeIdentity());
}

void xiiNonUniformBoxGizmo::OnSetOwner(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hOutline);

  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Nobs[i]);
  }
}

void xiiNonUniformBoxGizmo::OnVisibleChanged(bool bVisible)
{
  m_hOutline.SetVisible(bVisible);

  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    m_Nobs[i].SetVisible(bVisible);
  }
}

void xiiNonUniformBoxGizmo::OnTransformationChanged(const xiiTransform& transform)
{
  xiiMat4 scale, rot;
  scale = xiiMat4::MakeScaling(m_vNegSize + m_vPosSize);

  const xiiVec3 center = xiiMath::Lerp(-m_vNegSize, m_vPosSize, 0.5f);

  scale.SetTranslationVector(center);
  scale = transform.GetAsMat4() * scale;

  m_hOutline.SetTransformation(scale);

  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    xiiVec3 pos = center;
    xiiVec3 dir;

    switch (i)
    {
      case ManipulateMode::DragNegX:
        pos.x = -m_vNegSize.x;
        dir.Set(-1, 0, 0);
        break;
      case ManipulateMode::DragPosX:
        pos.x = m_vPosSize.x;
        dir.Set(+1, 0, 0);
        break;
      case ManipulateMode::DragNegY:
        pos.y = -m_vNegSize.y;
        dir.Set(0, -1, 0);
        break;
      case ManipulateMode::DragPosY:
        pos.y = m_vPosSize.y;
        dir.Set(0, +1, 0);
        break;
      case ManipulateMode::DragNegZ:
        pos.z = -m_vNegSize.z;
        dir.Set(0, 0, -1);
        break;
      case ManipulateMode::DragPosZ:
        pos.z = m_vPosSize.z;
        dir.Set(0, 0, +1);
        break;
    }
    pos = pos.CompMul(transform.m_vScale);

    xiiTransform t;
    t.m_qRotation = transform.m_qRotation;
    t.m_vPosition = transform.m_vPosition + t.m_qRotation * pos;
    t.m_vScale.Set(0.15f);

    m_vMainAxis[i] = t.m_qRotation * dir;

    m_Nobs[i].SetTransformation(t);
  }
}

void xiiNonUniformBoxGizmo::DoFocusLost(bool bCancel)
{
  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  xiiViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
}

xiiEditorInput xiiNonUniformBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return xiiEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return xiiEditorInput::MayBeHandledByOthers;

  for (xiiUInt32 i = 0; i < 6; ++i)
  {
    if (m_pInteractionGizmoHandle == &m_Nobs[i])
    {
      m_ManipulateMode = (ManipulateMode)i;
      m_vMoveAxis      = m_vMainAxis[i];
      m_vStartPosition = m_pInteractionGizmoHandle->GetTransformation().m_vPosition;
      goto modify;
    }
  }

  return xiiEditorInput::MayBeHandledByOthers;

modify:

{
  xiiMat4 mView = m_pCamera->GetViewMatrix();
  xiiMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  xiiMat4 mViewProj = mProj * mView;
  m_mInvViewProj    = mViewProj.GetInverse();
}

  m_vStartNegSize = m_vNegSize;
  m_vStartPosSize = m_vPosSize;

  GetPointOnAxis(e->pos().x(), m_vViewport.y - e->pos().y(), m_vInteractionPivot).IgnoreResult();

  xiiViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = xiiTime::Now();

  m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::Normal);

  SetActiveInputContext(this);

  m_fStartScale = (m_vInteractionPivot - m_pCamera->GetPosition()).GetLength() * 0.125;

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiNonUniformBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return xiiEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return xiiEditorInput::WasExclusivelyHandled;
}

xiiEditorInput xiiNonUniformBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return xiiEditorInput::MayBeHandledByOthers;

  const xiiTime tNow = xiiTime::Now();

  if (tNow - m_LastInteraction < xiiTime::MakeFromSeconds(1.0 / 25.0))
    return xiiEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  m_vNegSize = m_vStartNegSize;
  m_vPosSize = m_vStartPosSize;

  {
    xiiVec3 vCurrentInteractionPoint;

    if (GetPointOnAxis(e->pos().x(), m_vViewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
    {
      m_vLastMousePos = UpdateMouseMode(e);
      return xiiEditorInput::WasExclusivelyHandled;
    }

    const float   fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
    const xiiVec3 vOffset           = (m_vInteractionPivot - m_vStartPosition);

    const xiiVec3 vNewPos = vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale;

    xiiVec3 vTranslate = GetTransformation().m_qRotation.GetInverse() * (vNewPos - m_vStartPosition);

    // disable snapping when ALT is pressed
    if (!e->modifiers().testFlag(Qt::AltModifier))
    {
      xiiSnapProvider::SnapTranslation(vTranslate);
    }

    switch (m_ManipulateMode)
    {
      case None:
        break;
      case DragNegX:
        m_vNegSize.x -= vTranslate.x;
        if (m_bLinkAxis)
          m_vPosSize.x -= vTranslate.x;
        break;
      case DragPosX:
        m_vPosSize.x += vTranslate.x;
        if (m_bLinkAxis)
          m_vNegSize.x += vTranslate.x;
        break;
      case DragNegY:
        m_vNegSize.y -= vTranslate.y;
        if (m_bLinkAxis)
          m_vPosSize.y -= vTranslate.y;
        break;
      case DragPosY:
        m_vPosSize.y += vTranslate.y;
        if (m_bLinkAxis)
          m_vNegSize.y += vTranslate.y;
        break;
      case DragNegZ:
        m_vNegSize.z -= vTranslate.z;
        if (m_bLinkAxis)
          m_vPosSize.z -= vTranslate.z;
        break;
      case DragPosZ:
        m_vPosSize.z += vTranslate.z;
        if (m_bLinkAxis)
          m_vNegSize.z += vTranslate.z;
        break;
    }
  }

  m_vLastMousePos = UpdateMouseMode(e);

  // update the scale
  OnTransformationChanged(GetTransformation());

  xiiGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type   = xiiGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return xiiEditorInput::WasExclusivelyHandled;
}

void xiiNonUniformBoxGizmo::SetSize(const xiiVec3& vNegSize, const xiiVec3& vPosSize, bool bLinkAxis)
{
  m_vNegSize  = vNegSize;
  m_vPosSize  = vPosSize;
  m_bLinkAxis = bLinkAxis;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

xiiResult xiiNonUniformBoxGizmo::GetPointOnAxis(xiiInt32 iScreenPosX, xiiInt32 iScreenPosY, xiiVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  xiiVec3 vPos, vRayDir;
  if (xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, xiiVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return XII_FAILURE;

  const xiiVec3 vDir = m_pCamera->GetDirForwards();

  if (xiiMath::Abs(vDir.Dot(m_vMoveAxis)) > 0.999f)
    return XII_FAILURE;

  const xiiVec3 vPlaneTangent = m_vMoveAxis.CrossRH(vDir).GetNormalized();
  const xiiVec3 vPlaneNormal  = m_vMoveAxis.CrossRH(vPlaneTangent);

  xiiPlane Plane = xiiPlane::MakeFromNormalAndPoint(vPlaneNormal, m_vStartPosition);

  xiiVec3 vIntersection;
  if (m_pCamera->IsPerspective())
  {
    if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
      return XII_FAILURE;
  }
  else
  {
    if (!Plane.GetRayIntersectionBiDirectional(vPos - vRayDir, vRayDir, nullptr, &vIntersection))
      return XII_FAILURE;
  }

  const xiiVec3 vDirAlongRay     = vIntersection - m_vStartPosition;
  const float   fProjectedLength = vDirAlongRay.Dot(m_vMoveAxis);

  out_Result = m_vStartPosition + fProjectedLength * m_vMoveAxis;
  return XII_SUCCESS;
}
