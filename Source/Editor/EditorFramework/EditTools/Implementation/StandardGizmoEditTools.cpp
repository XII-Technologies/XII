#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/EditTools/StandardGizmoEditTools.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTranslateGizmoEditTool, 1, xiiRTTIDefaultAllocator<xiiTranslateGizmoEditTool>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiTranslateGizmoEditTool::xiiTranslateGizmoEditTool()
{
  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

xiiTranslateGizmoEditTool::~xiiTranslateGizmoEditTool()
{
  m_TranslateGizmo.m_GizmoEvents.RemoveEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::TransformationGizmoEventHandler, this));

  auto& events = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetDocument())->m_ChangedEvent;

  if (events.HasEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::OnPreferenceChange, this)))
    events.RemoveEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::OnPreferenceChange, this));
}

void xiiTranslateGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_TranslateGizmo.UpdateStatusBarText(GetWindow());
  }
}

void xiiTranslateGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_TranslateGizmo.SetOwner(GetWindow(), nullptr);

  xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetDocument())->m_ChangedEvent.AddEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::OnPreferenceChange, this));
}

void xiiTranslateGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_TranslateGizmo.SetVisible(visible);
}

void xiiTranslateGizmoEditTool::ApplyGizmoTransformation(const xiiTransform& transform)
{
  m_TranslateGizmo.SetTransformation(transform);
}

void xiiTranslateGizmoEditTool::TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e)
{
  xiiObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate = (QApplication::keyboardModifiers() == Qt::KeyboardModifier::ControlModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when CTRL is held while dragging the item
      if (bDuplicate && (e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<xiiOrthoGizmoContext>()))
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }

      if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
      {
        m_TranslateGizmo.SetMovementMode(xiiTranslateGizmo::MovementMode::MouseDiff);
      }
    }
    break;

    case xiiGizmoEvent::Type::Interaction:
    {
      auto         pDocument = GetDocument();
      xiiTransform tNew;

      if (e.m_pGizmo == &m_TranslateGizmo)
      {
        const xiiVec3 vTranslate = m_TranslateGizmo.GetTranslationResult();

        for (xiiUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        if (e.m_pGizmo == &m_TranslateGizmo && QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        {
          m_TranslateGizmo.SetMovementMode(xiiTranslateGizmo::MovementMode::MouseDiff);

          auto* pFocusedView = GetWindow()->GetFocusedViewWidget();
          if (pFocusedView != nullptr)
          {
            const xiiVec3 d = m_TranslateGizmo.GetTranslationDiff();
            pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(d.x, d.y, d.z);
          }
        }
        else
        {
          m_TranslateGizmo.SetMovementMode(xiiTranslateGizmo::MovementMode::ScreenProjection);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<xiiOrthoGizmoContext>())
      {
        const xiiOrthoGizmoContext* pOrtho = static_cast<const xiiOrthoGizmoContext*>(e.m_pGizmo);

        const xiiVec3 vTranslate = pOrtho->GetTranslationResult();

        for (xiiUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation);
        }

        if (QApplication::keyboardModifiers() & Qt::KeyboardModifier::ControlModifier)
        {
          // move the camera with the translated object

          auto* pFocusedView = GetWindow()->GetFocusedViewWidget();
          if (pFocusedView != nullptr)
          {
            const xiiVec3 d = pOrtho->GetTranslationDiff();
            pFocusedView->m_pViewConfig->m_Camera.MoveGlobally(d.x, d.y, d.z);
          }
        }
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

void xiiTranslateGizmoEditTool::OnPreferenceChange(xiiPreferences* pref)
{
  xiiScenePreferencesUser* pPref = xiiDynamicCast<xiiScenePreferencesUser*>(pref);

  m_TranslateGizmo.SetCameraSpeed(xiiCameraMoveContext::ConvertCameraSpeed(pPref->GetCameraSpeed()));
}

void xiiTranslateGizmoEditTool::GetGridSettings(xiiGridSettingsMsgToEngine& ref_msg)
{
  auto                     pSceneDoc    = GetDocument();
  xiiScenePreferencesUser* pPreferences = xiiPreferences::QueryPreferences<xiiScenePreferencesUser>(GetDocument());

  // if density != 0, it is enabled at least in ortho mode
  ref_msg.m_fGridDensity = xiiSnapProvider::GetTranslationSnapValue() * (pSceneDoc->GetGizmoWorldSpace() ? 1.0f : -1.0f); // negative density = local space

  // to be active in perspective mode, tangents have to be non-zero
  ref_msg.m_vGridTangent1.SetZero();
  ref_msg.m_vGridTangent2.SetZero();

  xiiTranslateGizmo& translateGizmo = m_TranslateGizmo;

  if (pPreferences->GetShowGrid() && translateGizmo.IsVisible())
  {
    ref_msg.m_vGridCenter = translateGizmo.GetStartPosition();

    switch (translateGizmo.GetLastHandleInteraction())
    {
      case xiiTranslateGizmo::HandleInteraction::AxisX:
        if (m_GridPlane == GridPlane::X)
          ref_msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;
        break;
      case xiiTranslateGizmo::HandleInteraction::AxisY:
        if (m_GridPlane == GridPlane::Y)
          ref_msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;
        break;
      case xiiTranslateGizmo::HandleInteraction::AxisZ:
        if (m_GridPlane == GridPlane::Z)
          ref_msg.m_vGridCenter = translateGizmo.GetTransformation().m_vPosition;
        break;
      case xiiTranslateGizmo::HandleInteraction::PlaneX:
        m_GridPlane = GridPlane::X;
        break;
      case xiiTranslateGizmo::HandleInteraction::PlaneY:
        m_GridPlane = GridPlane::Y;
        break;
      case xiiTranslateGizmo::HandleInteraction::PlaneZ:
        m_GridPlane = GridPlane::Z;
        break;
      case xiiTranslateGizmo::HandleInteraction::None:
        break;
    }

    if (pSceneDoc->GetGizmoWorldSpace())
    {
      switch (m_GridPlane)
      {
        case GridPlane::X:
          ref_msg.m_vGridCenter.y = xiiMath::RoundToMultiple(ref_msg.m_vGridCenter.y, xiiSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.z = xiiMath::RoundToMultiple(ref_msg.m_vGridCenter.z, xiiSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case GridPlane::Y:
          ref_msg.m_vGridCenter.x = xiiMath::RoundToMultiple(ref_msg.m_vGridCenter.x, xiiSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.z = xiiMath::RoundToMultiple(ref_msg.m_vGridCenter.z, xiiSnapProvider::GetTranslationSnapValue() * 10);
          break;
        case GridPlane::Z:
          ref_msg.m_vGridCenter.x = xiiMath::RoundToMultiple(ref_msg.m_vGridCenter.x, xiiSnapProvider::GetTranslationSnapValue() * 10);
          ref_msg.m_vGridCenter.y = xiiMath::RoundToMultiple(ref_msg.m_vGridCenter.y, xiiSnapProvider::GetTranslationSnapValue() * 10);
          break;
      }
    }

    switch (m_GridPlane)
    {
      case GridPlane::X:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * xiiVec3(0, 1, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * xiiVec3(0, 0, 1);
        break;
      case GridPlane::Y:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * xiiVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * xiiVec3(0, 0, 1);
        break;
      case GridPlane::Z:
        ref_msg.m_vGridTangent1 = translateGizmo.GetTransformation().m_qRotation * xiiVec3(1, 0, 0);
        ref_msg.m_vGridTangent2 = translateGizmo.GetTransformation().m_qRotation * xiiVec3(0, 1, 0);
        break;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRotateGizmoEditTool, 1, xiiRTTIDefaultAllocator<xiiRotateGizmoEditTool>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRotateGizmoEditTool::xiiRotateGizmoEditTool()
{
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

xiiRotateGizmoEditTool::~xiiRotateGizmoEditTool()
{
  m_RotateGizmo.m_GizmoEvents.RemoveEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void xiiRotateGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_RotateGizmo.SetOwner(GetWindow(), nullptr);
}

void xiiRotateGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_RotateGizmo.SetVisible(visible);
}

void xiiRotateGizmoEditTool::ApplyGizmoTransformation(const xiiTransform& transform)
{
  m_RotateGizmo.SetTransformation(transform);
}

void xiiRotateGizmoEditTool::TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e)
{
  xiiObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
        QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when CTRL is held while dragging the item
      if (e.m_pGizmo == &m_RotateGizmo && bDuplicate)
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }
    }
    break;

    case xiiGizmoEvent::Type::Interaction:
    {
      auto         pDocument = GetDocument();
      xiiTransform tNew;

      if (e.m_pGizmo == &m_RotateGizmo)
      {
        const xiiQuat qRotation = m_RotateGizmo.GetRotationResult();
        const xiiVec3 vPivot    = m_RotateGizmo.GetTransformation().m_vPosition;

        for (xiiUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew             = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;
          tNew.m_vPosition = vPivot + qRotation * (obj.m_GlobalTransform.m_vPosition - vPivot);

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<xiiOrthoGizmoContext>())
      {
        const xiiOrthoGizmoContext* pOrtho = static_cast<const xiiOrthoGizmoContext*>(e.m_pGizmo);

        const xiiQuat qRotation = pOrtho->GetRotationResult();

        for (xiiUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew             = obj.m_GlobalTransform;
          tNew.m_qRotation = qRotation * obj.m_GlobalTransform.m_qRotation;

          pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Rotation);
        }
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

void xiiRotateGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_RotateGizmo.UpdateStatusBarText(GetWindow());
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScaleGizmoEditTool, 1, xiiRTTIDefaultAllocator<xiiScaleGizmoEditTool>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiScaleGizmoEditTool::xiiScaleGizmoEditTool()
{
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

xiiScaleGizmoEditTool::~xiiScaleGizmoEditTool()
{
  m_ScaleGizmo.m_GizmoEvents.RemoveEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void xiiScaleGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_ScaleGizmo.UpdateStatusBarText(GetWindow());
  }
}

void xiiScaleGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_ScaleGizmo.SetOwner(GetWindow(), nullptr);
}

void xiiScaleGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_ScaleGizmo.SetVisible(visible);
}

void xiiScaleGizmoEditTool::ApplyGizmoTransformation(const xiiTransform& transform)
{
  m_ScaleGizmo.SetTransformation(transform);
}

void xiiScaleGizmoEditTool::TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e)
{
  xiiObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::Interaction:
    {
      xiiTransform tNew;

      bool bCancel = false;

      if (e.m_pGizmo == &m_ScaleGizmo)
      {
        const xiiVec3 vScale = m_ScaleGizmo.GetScalingResult();
        if (vScale.x == vScale.y && vScale.x == vScale.z)
        {
          for (xiiUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj       = m_GizmoSelection[sel];
            float       fNewScale = obj.m_fLocalUniformScaling * vScale.x;

            if (pAccessor->SetValueByName(obj.m_pObject, "LocalUniformScaling", fNewScale).Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
        else
        {
          for (xiiUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
          {
            const auto& obj       = m_GizmoSelection[sel];
            xiiVec3     vNewScale = obj.m_vLocalScaling.CompMul(vScale);

            if (pAccessor->SetValueByName(obj.m_pObject, "LocalScaling", vNewScale).Failed())
            {
              bCancel = true;
              break;
            }
          }
        }
      }

      if (e.m_pGizmo->GetDynamicRTTI()->IsDerivedFrom<xiiOrthoGizmoContext>())
      {
        const xiiOrthoGizmoContext* pOrtho = static_cast<const xiiOrthoGizmoContext*>(e.m_pGizmo);

        const float fScale = pOrtho->GetScalingResult();
        for (xiiUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj       = m_GizmoSelection[sel];
          const float fNewScale = obj.m_fLocalUniformScaling * fScale;

          if (pAccessor->SetValueByName(obj.m_pObject, "LocalUniformScaling", fNewScale).Failed())
          {
            bCancel = true;
            break;
          }
        }
      }

      if (bCancel)
        pAccessor->CancelTransaction();
      else
        pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDragToPositionGizmoEditTool, 1, xiiRTTIDefaultAllocator<xiiDragToPositionGizmoEditTool>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDragToPositionGizmoEditTool::xiiDragToPositionGizmoEditTool()
{

  m_DragToPosGizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

xiiDragToPositionGizmoEditTool::~xiiDragToPositionGizmoEditTool()
{
  m_DragToPosGizmo.m_GizmoEvents.RemoveEventHandler(xiiMakeDelegate(&xiiTranslateGizmoEditTool::TransformationGizmoEventHandler, this));
}

void xiiDragToPositionGizmoEditTool::OnActiveChanged(bool bIsActive)
{
  if (bIsActive)
  {
    m_DragToPosGizmo.UpdateStatusBarText(GetWindow());
  }
}

void xiiDragToPositionGizmoEditTool::OnConfigured()
{
  SUPER::OnConfigured();

  m_DragToPosGizmo.SetOwner(GetWindow(), nullptr);
}

void xiiDragToPositionGizmoEditTool::ApplyGizmoVisibleState(bool visible)
{
  m_DragToPosGizmo.SetVisible(visible);
}

void xiiDragToPositionGizmoEditTool::ApplyGizmoTransformation(const xiiTransform& transform)
{
  m_DragToPosGizmo.SetTransformation(transform);
}

void xiiDragToPositionGizmoEditTool::TransformationGizmoEventHandlerImpl(const xiiGizmoEvent& e)
{
  xiiObjectAccessorBase* pAccessor = GetGizmoInterface()->GetObjectAccessor();
  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::BeginInteractions:
    {
      const bool bDuplicate =
        QApplication::keyboardModifiers().testFlag(Qt::KeyboardModifier::ControlModifier) && GetGizmoInterface()->CanDuplicateSelection();

      // duplicate the object when CTRL is held while dragging the item
      if (e.m_pGizmo == &m_DragToPosGizmo && bDuplicate)
      {
        m_bMergeTransactions = true;
        GetGizmoInterface()->DuplicateSelection();
      }
    }
    break;

    case xiiGizmoEvent::Type::Interaction:
    {
      auto         pDocument = GetDocument();
      xiiTransform tNew;

      if (e.m_pGizmo == &m_DragToPosGizmo)
      {
        const xiiVec3 vTranslate = m_DragToPosGizmo.GetTranslationResult();
        const xiiQuat qRot       = m_DragToPosGizmo.GetRotationResult();

        for (xiiUInt32 sel = 0; sel < m_GizmoSelection.GetCount(); ++sel)
        {
          const auto& obj = m_GizmoSelection[sel];

          tNew = obj.m_GlobalTransform;
          tNew.m_vPosition += vTranslate;

          if (m_DragToPosGizmo.ModifiesRotation())
          {
            tNew.m_qRotation = qRot;
          }

          if (GetDocument()->GetGizmoMoveParentOnly())
            pDocument->SetGlobalTransformParentOnly(obj.m_pObject, tNew, TransformationChanges::Rotation | TransformationChanges::Translation);
          else
            pDocument->SetGlobalTransform(obj.m_pObject, tNew, TransformationChanges::Translation | TransformationChanges::Rotation);
        }
      }

      pAccessor->FinishTransaction();
    }
    break;

    default:
      break;
  }
}
