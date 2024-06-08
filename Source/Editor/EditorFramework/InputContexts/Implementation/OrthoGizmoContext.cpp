#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiOrthoGizmoContext, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiOrthoGizmoContext::xiiOrthoGizmoContext(xiiQtEngineDocumentWindow* pOwnerWindow, xiiQtEngineViewWidget* pOwnerView, const xiiCamera* pCamera)
{
  m_pCamera      = pCamera;
  m_bCanInteract = false;

  SetOwner(pOwnerWindow, pOwnerView);
}

void xiiOrthoGizmoContext::FocusLost(bool bCancel)
{
  xiiGizmoEvent e;
  e.m_pGizmo = this;
  e.m_Type   = bCancel ? xiiGizmoEvent::Type::CancelInteractions : xiiGizmoEvent::Type::EndInteractions;

  m_GizmoEvents.Broadcast(e);

  m_bCanInteract = false;
  SetActiveInputContext(nullptr);

  xiiEditorInputContext::FocusLost(bCancel);
}

xiiEditorInput xiiOrthoGizmoContext::DoMousePressEvent(QMouseEvent* e)
{
  if (!IsViewInOrthoMode())
    return xiiEditorInput::MayBeHandledByOthers;
  if (GetOwnerWindow()->GetDocument()->GetSelectionManager()->IsSelectionEmpty())
    return xiiEditorInput::MayBeHandledByOthers;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bCanInteract = true;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiOrthoGizmoContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
  {
    m_bCanInteract = false;
    return xiiEditorInput::MayBeHandledByOthers;
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    FocusLost(false);
    return xiiEditorInput::WasExclusivelyHandled;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

xiiEditorInput xiiOrthoGizmoContext::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!e->buttons().testFlag(Qt::MouseButton::LeftButton))
  {
    m_bCanInteract = false;
    return xiiEditorInput::MayBeHandledByOthers;
  }

  if (IsActiveInputContext())
  {
    float fDistPerPixel = 0;

    if (m_pCamera->GetCameraMode() == xiiCameraMode::OrthoFixedHeight)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().height();

    if (m_pCamera->GetCameraMode() == xiiCameraMode::OrthoFixedWidth)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().width();

    const xiiVec3 vLastTranslationResult = m_vTranslationResult;

    const QPoint mousePosition = e->globalPosition().toPoint();

    const xiiVec2I32 diff = xiiVec2I32(mousePosition.x(), mousePosition.y()) - m_vLastMousePos;

    m_vUnsnappedTranslationResult += m_pCamera->GetDirRight() * (float)diff.x * fDistPerPixel;
    m_vUnsnappedTranslationResult -= m_pCamera->GetDirUp() * (float)diff.y * fDistPerPixel;

    m_vTranslationResult = m_vUnsnappedTranslationResult;

    // disable snapping when ALT is pressed
    if (!e->modifiers().testFlag(Qt::AltModifier))
      xiiSnapProvider::SnapTranslation(m_vTranslationResult);

    m_vTranslationDiff = m_vTranslationResult - vLastTranslationResult;

    m_UnsnappedRotationResult += xiiAngle::MakeFromDegree(-diff.x);

    xiiAngle snappedRotation = m_UnsnappedRotationResult;

    // disable snapping when ALT is pressed
    if (!e->modifiers().testFlag(Qt::AltModifier))
      xiiSnapProvider::SnapRotation(snappedRotation);

    m_qRotationResult.SetFromAxisAndAngle(m_pCamera->GetDirForwards(), snappedRotation);

    {
      m_fScaleMouseMove += diff.x;
      m_fUnsnappedScalingResult = 1.0f;

      const float fScaleSpeed = 0.01f;

      if (m_fScaleMouseMove > 0.0f)
        m_fUnsnappedScalingResult = 1.0f + m_fScaleMouseMove * fScaleSpeed;
      if (m_fScaleMouseMove < 0.0f)
        m_fUnsnappedScalingResult = 1.0f / (1.0f - m_fScaleMouseMove * fScaleSpeed);

      m_fScalingResult = m_fUnsnappedScalingResult;

      // disable snapping when ALT is pressed
      if (!e->modifiers().testFlag(Qt::AltModifier))
        xiiSnapProvider::SnapScale(m_fScalingResult);
    }

    m_vLastMousePos = UpdateMouseMode(e);

    xiiGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type   = xiiGizmoEvent::Type::Interaction;

    m_GizmoEvents.Broadcast(ev);

    return xiiEditorInput::WasExclusivelyHandled;
  }

  if (m_bCanInteract)
  {
    m_vLastMousePos = SetMouseMode(xiiEditorInputContext::MouseMode::WrapAtScreenBorders);
    m_vTranslationResult.SetZero();
    m_vUnsnappedTranslationResult.SetZero();
    m_qRotationResult.SetIdentity();
    m_UnsnappedRotationResult = xiiAngle::MakeFromRadian(0.0f);
    m_fScalingResult          = 1.0f;
    m_fUnsnappedScalingResult = 1.0f;
    m_fScaleMouseMove         = 0.0f;

    m_bCanInteract = false;
    SetActiveInputContext(this);

    xiiGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type   = xiiGizmoEvent::Type::BeginInteractions;

    m_GizmoEvents.Broadcast(ev);
    return xiiEditorInput::WasExclusivelyHandled;
  }

  return xiiEditorInput::MayBeHandledByOthers;
}

bool xiiOrthoGizmoContext::IsViewInOrthoMode() const
{
  return (GetOwnerView()->m_pViewConfig->m_Perspective != xiiSceneViewPerspective::Perspective);
}
