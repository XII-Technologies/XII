#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/ConeLengthManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiConeLengthManipulatorAdapter::xiiConeLengthManipulatorAdapter() = default;

xiiConeLengthManipulatorAdapter::~xiiConeLengthManipulatorAdapter() = default;

void xiiConeLengthManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = xiiQtDocumentWindow::FindWindowByDocument(pDoc);

  xiiQtEngineDocumentWindow* pEngineWindow = qobject_cast<xiiQtEngineDocumentWindow*>(pWindow);
  XII_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiConeLengthManipulatorAdapter::GizmoEventHandler, this));
}

void xiiConeLengthManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  xiiObjectAccessorBase*                   pObjectAccessor = GetObjectAccessor();
  const xiiConeLengthManipulatorAttribute* pAttr           = static_cast<const xiiConeLengthManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
    m_Gizmo.SetRadius(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void xiiConeLengthManipulatorAdapter::GizmoEventHandler(const xiiGizmoEvent& e)
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
      const xiiConeLengthManipulatorAttribute* pAttr = static_cast<const xiiConeLengthManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetRadiusProperty(), m_Gizmo.GetRadius());
    }
    break;
  }
}

void xiiConeLengthManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
