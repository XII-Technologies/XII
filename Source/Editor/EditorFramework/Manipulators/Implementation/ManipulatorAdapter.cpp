#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Manipulators/ManipulatorAdapter.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiManipulatorAdapter::xiiManipulatorAdapter()
{
  m_pManipulatorAttr      = nullptr;
  m_pObject               = nullptr;
  m_bManipulatorIsVisible = true;

  xiiQtDocumentWindow::s_Events.AddEventHandler(xiiMakeDelegate(&xiiManipulatorAdapter::DocumentWindowEventHandler, this));
}

xiiManipulatorAdapter::~xiiManipulatorAdapter()
{
  xiiQtDocumentWindow::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiManipulatorAdapter::DocumentWindowEventHandler, this));

  if (m_pObject)
  {
    m_pObject->GetDocumentObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
    m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(xiiMakeDelegate(&xiiManipulatorAdapter::DocumentObjectMetaDataEventHandler, this));
  }
}

void xiiManipulatorAdapter::SetManipulator(const xiiManipulatorAttribute* pAttribute, const xiiDocumentObject* pObject)
{
  m_pManipulatorAttr = pAttribute;
  m_pObject          = pObject;

  auto& meta = *m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument()->m_DocumentObjectMetaData;

  m_pObject->GetDocumentObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiManipulatorAdapter::DocumentObjectPropertyEventHandler, this));
  meta.m_DataModifiedEvent.AddEventHandler(xiiMakeDelegate(&xiiManipulatorAdapter::DocumentObjectMetaDataEventHandler, this));

  {
    auto pMeta              = meta.BeginReadMetaData(m_pObject->GetGuid());
    m_bManipulatorIsVisible = !pMeta->m_bHidden;
    meta.EndReadMetaData();
  }

  Finalize();

  Update();
}

void xiiManipulatorAdapter::DocumentObjectPropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == xiiDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_pObject == m_pObject)
    {
      if (e.m_sProperty == m_pManipulatorAttr->m_sProperty1 || e.m_sProperty == m_pManipulatorAttr->m_sProperty2 || e.m_sProperty == m_pManipulatorAttr->m_sProperty3 || e.m_sProperty == m_pManipulatorAttr->m_sProperty4 || e.m_sProperty == m_pManipulatorAttr->m_sProperty5 ||
          e.m_sProperty == m_pManipulatorAttr->m_sProperty6)
      {
        Update();
      }
    }
  }
}

void xiiManipulatorAdapter::DocumentWindowEventHandler(const xiiQtDocumentWindowEvent& e)
{
  if (e.m_Type == xiiQtDocumentWindowEvent::BeforeRedraw && e.m_pWindow->GetDocument() == m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument())
  {
    UpdateGizmoTransform();
  }
}

void xiiManipulatorAdapter::DocumentObjectMetaDataEventHandler(const xiiObjectMetaData<xiiUuid, xiiDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & xiiDocumentObjectMetaData::HiddenFlag) != 0 && e.m_ObjectKey == m_pObject->GetGuid())
  {
    m_bManipulatorIsVisible = !e.m_pValue->m_bHidden;

    Update();
  }
}

xiiTransform xiiManipulatorAdapter::GetOffsetTransform() const
{
  return xiiTransform::IdentityTransform();
}

xiiTransform xiiManipulatorAdapter::GetObjectTransform() const
{
  xiiTransform tObj;
  m_pObject->GetDocumentObjectManager()->GetDocument()->ComputeObjectTransformation(m_pObject, tObj).IgnoreResult();

  const xiiTransform offset = GetOffsetTransform();

  xiiTransform tGlobal;
  tGlobal.SetGlobalTransform(tObj, offset);

  return tGlobal;
}

xiiObjectAccessorBase* xiiManipulatorAdapter::GetObjectAccessor() const
{
  return m_pObject->GetDocumentObjectManager()->GetDocument()->GetObjectAccessor();
}

const xiiAbstractProperty* xiiManipulatorAdapter::GetProperty(const char* szProperty) const
{
  return m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(szProperty);
}

void xiiManipulatorAdapter::BeginTemporaryInteraction()
{
  GetObjectAccessor()->BeginTemporaryCommands("Adjust Object");
}

void xiiManipulatorAdapter::EndTemporaryInteraction()
{
  GetObjectAccessor()->FinishTemporaryCommands();
}

void xiiManipulatorAdapter::CancelTemporayInteraction()
{
  GetObjectAccessor()->CancelTemporaryCommands();
}

void xiiManipulatorAdapter::ClampProperty(const char* szProperty, xiiVariant& value) const
{
  xiiResult    status(XII_FAILURE);
  const double fCur = value.ConvertTo<double>(&status);

  if (status.Failed())
    return;

  const xiiClampValueAttribute* pClamp = GetProperty(szProperty)->GetAttributeByType<xiiClampValueAttribute>();
  if (pClamp == nullptr)
    return;

  if (pClamp->GetMinValue().IsValid())
  {
    const double fMin = pClamp->GetMinValue().ConvertTo<double>(&status);
    if (status.Succeeded())
    {
      if (fCur < fMin)
        value = pClamp->GetMinValue();
    }
  }

  if (pClamp->GetMaxValue().IsValid())
  {
    const double fMax = pClamp->GetMaxValue().ConvertTo<double>(&status);
    if (status.Succeeded())
    {
      if (fCur > fMax)
        value = pClamp->GetMaxValue();
    }
  }
}

void xiiManipulatorAdapter::ChangeProperties(const char* szProperty1, xiiVariant value1, const char* szProperty2 /*= nullptr*/, xiiVariant value2 /*= xiiVariant()*/, const char* szProperty3 /*= nullptr*/, xiiVariant value3 /*= xiiVariant()*/, const char* szProperty4 /*= nullptr*/, xiiVariant value4 /*= xiiVariant()*/, const char* szProperty5 /*= nullptr*/, xiiVariant value5 /*= xiiVariant()*/, const char* szProperty6 /*= nullptr*/, xiiVariant value6 /*= xiiVariant()*/)
{
  xiiObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  pObjectAccessor->StartTransaction("Change Properties");

  if (!xiiStringUtils::IsNullOrEmpty(szProperty1))
  {
    ClampProperty(szProperty1, value1);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty1), value1);
  }

  if (!xiiStringUtils::IsNullOrEmpty(szProperty2))
  {
    ClampProperty(szProperty2, value2);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty2), value2);
  }

  if (!xiiStringUtils::IsNullOrEmpty(szProperty3))
  {
    ClampProperty(szProperty3, value3);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty3), value3);
  }

  if (!xiiStringUtils::IsNullOrEmpty(szProperty4))
  {
    ClampProperty(szProperty4, value4);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty4), value4);
  }

  if (!xiiStringUtils::IsNullOrEmpty(szProperty5))
  {
    ClampProperty(szProperty5, value5);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty5), value5);
  }

  if (!xiiStringUtils::IsNullOrEmpty(szProperty6))
  {
    ClampProperty(szProperty6, value6);
    pObjectAccessor->SetValue(m_pObject, GetProperty(szProperty6), value6);
  }

  pObjectAccessor->FinishTransaction();
}
