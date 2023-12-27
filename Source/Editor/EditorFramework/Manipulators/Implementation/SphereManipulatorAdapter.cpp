#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/SphereManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiSphereManipulatorAdapter::xiiSphereManipulatorAdapter() = default;

xiiSphereManipulatorAdapter::~xiiSphereManipulatorAdapter() = default;

void xiiSphereManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = xiiQtDocumentWindow::FindWindowByDocument(pDoc);

  xiiQtEngineDocumentWindow* pEngineWindow = qobject_cast<xiiQtEngineDocumentWindow*>(pWindow);
  XII_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiSphereManipulatorAdapter::GizmoEventHandler, this));
}

void xiiSphereManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  xiiObjectAccessorBase*               pObjectAccessor = GetObjectAccessor();
  const xiiSphereManipulatorAttribute* pAttr           = static_cast<const xiiSphereManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetInnerRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetInnerRadiusProperty()));
    m_Gizmo.SetInnerSphere(true, fValue);
  }

  if (!pAttr->GetOuterRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetOuterRadiusProperty()));
    m_Gizmo.SetOuterSphere(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void xiiSphereManipulatorAdapter::GizmoEventHandler(const xiiGizmoEvent& e)
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
      const xiiSphereManipulatorAttribute* pAttr = static_cast<const xiiSphereManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetInnerRadiusProperty(), m_Gizmo.GetInnerRadius(), pAttr->GetOuterRadiusProperty(), m_Gizmo.GetOuterRadius());
    }
    break;
  }
}

void xiiSphereManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
