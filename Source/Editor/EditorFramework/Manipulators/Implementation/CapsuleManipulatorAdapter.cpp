#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiCapsuleManipulatorAdapter::xiiCapsuleManipulatorAdapter() {}

xiiCapsuleManipulatorAdapter::~xiiCapsuleManipulatorAdapter() {}

void xiiCapsuleManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = xiiQtDocumentWindow::FindWindowByDocument(pDoc);

  xiiQtEngineDocumentWindow* pEngineWindow = qobject_cast<xiiQtEngineDocumentWindow*>(pWindow);
  XII_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(xiiMakeDelegate(&xiiCapsuleManipulatorAdapter::GizmoEventHandler, this));
}

void xiiCapsuleManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  xiiObjectAccessorBase*                pObjectAccessor = GetObjectAccessor();
  const xiiCapsuleManipulatorAttribute* pAttr           = static_cast<const xiiCapsuleManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetLengthProperty()));
    m_Gizmo.SetLength(fValue);
  }

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
    m_Gizmo.SetRadius(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void xiiCapsuleManipulatorAdapter::GizmoEventHandler(const xiiGizmoEvent& e)
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
      const xiiCapsuleManipulatorAttribute* pAttr = static_cast<const xiiCapsuleManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetLengthProperty(), m_Gizmo.GetLength(), pAttr->GetRadiusProperty(), m_Gizmo.GetRadius());
    }
    break;
  }
}

void xiiCapsuleManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
