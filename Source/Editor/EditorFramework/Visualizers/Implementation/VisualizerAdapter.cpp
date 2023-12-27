#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

xiiVisualizerAdapter::xiiVisualizerAdapter()
{
  m_pVisualizerAttr      = nullptr;
  m_pObject              = nullptr;
  m_bVisualizerIsVisible = true;

  xiiQtDocumentWindow::s_Events.AddEventHandler(xiiMakeDelegate(&xiiVisualizerAdapter::DocumentWindowEventHandler, this));
}

xiiVisualizerAdapter::~xiiVisualizerAdapter()
{
  xiiQtDocumentWindow::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
    m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiVisualizerAdapter::DocumentObjectMetaDataEventHandler, this));
  }
}

void xiiVisualizerAdapter::SetVisualizer(const xiiVisualizerAttribute* pAttribute, const xiiDocumentObject* pObject)
{
  m_pVisualizerAttr = pAttribute;
  m_pObject         = pObject;

  auto& meta = *m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiVisualizerAdapter::DocumentObjectPropertyEventHandler, this));
  meta.m_DataModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiVisualizerAdapter::DocumentObjectMetaDataEventHandler, this));

  {
    auto pMeta             = meta.BeginReadMetaData(m_pObject->GetGuid());
    m_bVisualizerIsVisible = !pMeta->m_bHidden;
    meta.EndReadMetaData();
  }

  Finalize();

  Update();
}

void xiiVisualizerAdapter::DocumentObjectPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == xiiDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sProperty == m_pVisualizerAttr->m_sProperty1 || e.m_sProperty == m_pVisualizerAttr->m_sProperty2 || e.m_sProperty == m_pVisualizerAttr->m_sProperty3 || e.m_sProperty == m_pVisualizerAttr->m_sProperty4 || e.m_sProperty == m_pVisualizerAttr->m_sProperty5)
      {
        Update();
      }
    }
  }
}

void xiiVisualizerAdapter::DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e)
{
  if (e.m_Type == xiiQtDocumentWindowEvent::BeforeRedraw && e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument())
  {
    UpdateGizmoTransform();
  }
}

void xiiVisualizerAdapter::DocumentObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & xiiDocumentObjectMetaData::HiddenFlag) != 0 && e.m_ObjectKey == m_pObject->GetGuid())
  {
    m_bVisualizerIsVisible = !e.m_pValue->m_bHidden;

    Update();
  }
}

xiiTransform xiiVisualizerAdapter::GetObjectTransform() const
{
  xiiTransform t;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, t).IgnoreResult();

  return t;
}

xiiObjectAccessorBase* xiiVisualizerAdapter::GetObjectAccessor() const
{
  return m_pObject->GetDocumentObjectManager()->GetDocument()->GetObjectAccessor();
}

const xiiAbstractProperty* xiiVisualizerAdapter::GetProperty(const char* szProperty) const
{
  return m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
}
