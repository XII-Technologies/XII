/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/TransformManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiTransformManipulatorAdapter::xiiTransformManipulatorAdapter() = default;

xiiTransformManipulatorAdapter::~xiiTransformManipulatorAdapter() = default;

void xiiTransformManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = xiiQtDocumentWindow::FindWindowByDocument(pDoc);

  xiiQtEngineDocumentWindow* pEngineWindow = qobject_cast<xiiQtEngineDocumentWindow*>(pWindow);
  XII_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_TranslateGizmo.SetTransformation(GetObjectTransform());
  m_RotateGizmo.SetTransformation(GetObjectTransform());
  m_ScaleGizmo.SetTransformation(GetObjectTransform());

  const xiiTransformManipulatorAttribute* pAttr = static_cast<const xiiTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetTranslateProperty().IsEmpty())
  {
    m_bHideTranslate = GetProperty(pAttr->GetTranslateProperty())->GetFlags().IsSet(xiiPropertyFlags::ReadOnly);
  }

  if (!pAttr->GetRotateProperty().IsEmpty())
  {
    m_bHideRotate = GetProperty(pAttr->GetRotateProperty())->GetFlags().IsSet(xiiPropertyFlags::ReadOnly);
  }

  if (!pAttr->GetScaleProperty().IsEmpty())
  {
    m_bHideScale = GetProperty(pAttr->GetScaleProperty())->GetFlags().IsSet(xiiPropertyFlags::ReadOnly);
  }

  m_TranslateGizmo.SetOwner(pEngineWindow, nullptr);
  m_TranslateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideTranslate);
  m_RotateGizmo.SetOwner(pEngineWindow, nullptr);
  m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideRotate);
  m_ScaleGizmo.SetOwner(pEngineWindow, nullptr);
  m_ScaleGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideScale);

  m_TranslateGizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiTransformManipulatorAdapter::GizmoEventHandler, this));
  m_RotateGizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiTransformManipulatorAdapter::GizmoEventHandler, this));
  m_ScaleGizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiTransformManipulatorAdapter::GizmoEventHandler, this));
}

void xiiTransformManipulatorAdapter::Update()
{
  UpdateGizmoTransform();
}

void xiiTransformManipulatorAdapter::GizmoEventHandler(const xiiGizmoEvent& e)
{
  const xiiTransformManipulatorAttribute* pAttr = static_cast<const xiiTransformManipulatorAttribute*>(m_pManipulatorAttr);

  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::BeginInteractions:
      m_vOldScale = GetScale();
      BeginTemporaryInteraction();
      break;

    case xiiGizmoEvent::Type::CancelInteractions:
      CancelTemporayInteraction();
      break;

    case xiiGizmoEvent::Type::EndInteractions:
      EndTemporaryInteraction();
      break;

    case xiiGizmoEvent::Type::Interaction:
    {
      if (e.m_pGizmo == &m_TranslateGizmo || e.m_pGizmo == &m_RotateGizmo || e.m_pGizmo == &m_ScaleGizmo)
      {
        const xiiTransform tParent = GetObjectTransform();
        const xiiTransform tGlobal = static_cast<const xiiGizmo*>(e.m_pGizmo)->GetTransformation();
        xiiTransform       tLocal  = xiiTransform::MakeLocalTransform(tParent, tGlobal);
        if (e.m_pGizmo == &m_TranslateGizmo)
        {
          ChangeProperties(pAttr->GetTranslateProperty(), tLocal.m_vPosition);
        }
        else if (e.m_pGizmo == &m_RotateGizmo)
        {
          ChangeProperties(pAttr->GetRotateProperty(), tLocal.m_qRotation);
        }
        else if (e.m_pGizmo == &m_ScaleGizmo)
        {
          xiiVec3 vNewScale = m_vOldScale.CompMul(m_ScaleGizmo.GetScalingResult());
          ChangeProperties(pAttr->GetScaleProperty(), vNewScale);
        }
      }
    }
    break;
  }
}


void xiiTransformManipulatorAdapter::UpdateGizmoTransform()
{
  m_TranslateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideTranslate);
  m_RotateGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideRotate);
  m_ScaleGizmo.SetVisible(m_bManipulatorIsVisible && !m_bHideScale);

  const xiiVec3 vPos   = GetTranslation();
  const xiiQuat vRot   = GetRotation();
  const xiiVec3 vScale = GetScale();

  const xiiTransform tParent = GetObjectTransform();
  xiiTransform       tLocal;
  tLocal.m_vPosition   = vPos;
  tLocal.m_qRotation   = vRot;
  tLocal.m_vScale      = vScale;
  xiiTransform tGlobal = xiiTransform::MakeGlobalTransform(tParent, tLocal);
  // Let's not apply scaling to the gizmos.
  tGlobal.m_vScale = xiiVec3(1, 1, 1);

  m_TranslateGizmo.SetTransformation(tGlobal);
  m_RotateGizmo.SetTransformation(tGlobal);
  m_ScaleGizmo.SetTransformation(tGlobal);
}

xiiVec3 xiiTransformManipulatorAdapter::GetTranslation()
{
  const xiiTransformManipulatorAttribute* pAttr = static_cast<const xiiTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetTranslateProperty().IsEmpty())
  {
    xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetTranslateProperty()));
  }

  return xiiVec3(0);
}

xiiQuat xiiTransformManipulatorAdapter::GetRotation()
{
  const xiiTransformManipulatorAttribute* pAttr = static_cast<const xiiTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRotateProperty().IsEmpty())
  {
    xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<xiiQuat>(m_pObject, GetProperty(pAttr->GetRotateProperty()));
  }

  return xiiQuat::MakeIdentity();
}

xiiVec3 xiiTransformManipulatorAdapter::GetScale()
{
  const xiiTransformManipulatorAttribute* pAttr = static_cast<const xiiTransformManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetScaleProperty().IsEmpty())
  {
    xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
    return pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetScaleProperty()));
  }

  return xiiVec3(1);
}

xiiTransform xiiTransformManipulatorAdapter::GetOffsetTransform() const
{
  xiiTransform offset;
  offset.SetIdentity();

  if (const xiiTransformManipulatorAttribute* pAttr = xiiDynamicCast<const xiiTransformManipulatorAttribute*>(m_pManipulatorAttr))
  {
    if (!pAttr->GetGetOffsetTranslationProperty().IsEmpty())
    {
      xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
      offset.m_vPosition                     = pObjectAccessor->Get<xiiVec3>(m_pObject, GetProperty(pAttr->GetGetOffsetTranslationProperty()));
    }

    if (!pAttr->GetGetOffsetRotationProperty().IsEmpty())
    {
      xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
      offset.m_qRotation                     = pObjectAccessor->Get<xiiQuat>(m_pObject, GetProperty(pAttr->GetGetOffsetRotationProperty()));
    }
  }

  return offset;
}
