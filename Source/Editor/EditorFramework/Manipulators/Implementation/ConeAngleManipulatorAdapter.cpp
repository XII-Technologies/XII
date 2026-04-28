/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/ConeAngleManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiConeAngleManipulatorAdapter::xiiConeAngleManipulatorAdapter() = default;

xiiConeAngleManipulatorAdapter::~xiiConeAngleManipulatorAdapter() = default;

void xiiConeAngleManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = xiiQtDocumentWindow::FindWindowByDocument(pDoc);

  xiiQtEngineDocumentWindow* pEngineWindow = qobject_cast<xiiQtEngineDocumentWindow*>(pWindow);
  XII_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiConeAngleManipulatorAdapter::GizmoEventHandler, this));
}

void xiiConeAngleManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  xiiObjectAccessorBase*                  pObjectAccessor = GetObjectAccessor();
  const xiiConeAngleManipulatorAttribute* pAttr           = static_cast<const xiiConeAngleManipulatorAttribute*>(m_pManipulatorAttr);

  /* if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
  } */

  m_Gizmo.SetRadius(pAttr->m_fScale);

  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    xiiAngle value = pObjectAccessor->Get<xiiAngle>(m_pObject, GetProperty(pAttr->GetAngleProperty()));
    m_Gizmo.SetAngle(value);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void xiiConeAngleManipulatorAdapter::GizmoEventHandler(const xiiGizmoEvent& e)
{
  switch (e.m_Type)
  {
    case xiiGizmoEvent::Type::BeginInteractions:
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
      const xiiConeAngleManipulatorAttribute* pAttr = static_cast<const xiiConeAngleManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetAngleProperty(), m_Gizmo.GetAngle());
    }
    break;
  }
}

void xiiConeAngleManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
