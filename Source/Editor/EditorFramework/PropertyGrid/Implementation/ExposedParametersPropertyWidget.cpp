#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>

xiiExposedParameterCommandAccessor::xiiExposedParameterCommandAccessor(xiiObjectAccessorBase* pSource, const xiiAbstractProperty* pParameterProp, const xiiAbstractProperty* pParameterSourceProp) :
  xiiObjectProxyAccessor(pSource), m_pParameterProp(pParameterProp), m_pParameterSourceProp(pParameterSourceProp)
{
}

xiiStatus xiiExposedParameterCommandAccessor::GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index /*= xiiVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  xiiStatus res = xiiObjectProxyAccessor::GetValue(pObject, pProp, out_value, index);
  if (res.Succeeded() && !index.IsValid() && m_pParameterProp == pProp)
  {
    xiiVariantDictionary defaultDict;
    if (const xiiExposedParameters* pParams = GetExposedParams(pObject))
    {
      for (xiiExposedParameter* pParam : pParams->m_Parameters)
      {
        defaultDict.Insert(pParam->m_sName, pParam->m_DefaultValue);
      }
    }
    const xiiVariantDictionary& overwrittenDict = out_value.Get<xiiVariantDictionary>();
    for (auto it : overwrittenDict)
    {
      defaultDict[it.Key()] = it.Value();
    }
    out_value = defaultDict;
  }
  else if (res.Failed() && m_pParameterProp == pProp && index.IsA<xiiString>())
  {
    // If the actual GetValue fails but the key is an exposed param, return its default value instead.
    if (const xiiExposedParameter* pParam = GetExposedParam(pObject, index.Get<xiiString>()))
    {
      out_value = pParam->m_DefaultValue;
      return xiiStatus(XII_SUCCESS);
    }
  }
  return res;
}

xiiStatus xiiExposedParameterCommandAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index /*= xiiVariant()*/)
{
  if (IsExposedProperty(pObject, pProp))
    pProp = m_pParameterProp;

  xiiStatus res = xiiObjectProxyAccessor::SetValue(pObject, pProp, newValue, index);
  // As we pretend the exposed params always exist the actual SetValue will fail if this is not actually true,
  // so we redirect to insert to make it true.
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<xiiString>())
  {
    return InsertValue(pObject, pProp, newValue, index);
  }
  return res;
}

xiiStatus xiiExposedParameterCommandAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index /*= xiiVariant()*/)
{
  xiiStatus res = xiiObjectProxyAccessor::RemoveValue(pObject, pProp, index);
  if (res.Failed() && m_pParameterProp == pProp && index.IsA<xiiString>())
  {
    // It this is one of the exposed params, pretend we removed it successfully to suppress error messages.
    if (const xiiExposedParameter* pParam = GetExposedParam(pObject, index.Get<xiiString>()))
    {
      return xiiStatus(XII_SUCCESS);
    }
  }
  return res;
}

xiiStatus xiiExposedParameterCommandAccessor::GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount)
{
  if (m_pParameterProp == pProp)
  {
    xiiHybridArray<xiiVariant, 16> keys;
    GetKeys(pObject, pProp, keys).AssertSuccess();
    out_iCount = keys.GetCount();
    return xiiStatus(XII_SUCCESS);
  }
  return xiiObjectProxyAccessor::GetCount(pObject, pProp, out_iCount);
}

xiiStatus xiiExposedParameterCommandAccessor::GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys)
{
  if (m_pParameterProp == pProp)
  {
    if (const xiiExposedParameters* pParams = GetExposedParams(pObject))
    {
      for (const auto& pParam : pParams->m_Parameters)
      {
        out_keys.PushBack(xiiVariant(pParam->m_sName));
      }

      xiiHybridArray<xiiVariant, 16> realKeys;
      xiiStatus                      res = xiiObjectProxyAccessor::GetKeys(pObject, pProp, realKeys);
      for (const auto& key : realKeys)
      {
        if (!out_keys.Contains(key))
        {
          out_keys.PushBack(key);
        }
      }
      return xiiStatus(XII_SUCCESS);
    }
  }
  return xiiObjectProxyAccessor::GetKeys(pObject, pProp, out_keys);
}

xiiStatus xiiExposedParameterCommandAccessor::GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values)
{
  if (m_pParameterProp == pProp)
  {
    xiiHybridArray<xiiVariant, 16> keys;
    GetKeys(pObject, pProp, keys).AssertSuccess();
    for (const auto& key : keys)
    {
      auto& var = out_values.ExpandAndGetRef();
      XII_VERIFY(GetValue(pObject, pProp, var, key).Succeeded(), "GetValue to valid a key should be not fail.");
    }
    return xiiStatus(XII_SUCCESS);
  }
  return xiiObjectProxyAccessor::GetValues(pObject, pProp, out_values);
}

const xiiExposedParameters* xiiExposedParameterCommandAccessor::GetExposedParams(const xiiDocumentObject* pObject)
{
  xiiVariant value;
  if (xiiObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<xiiString>())
    {
      const auto& sValue = value.Get<xiiString>();
      if (const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo<xiiExposedParameters>();
      }
    }
  }
  return nullptr;
}

const xiiExposedParameter* xiiExposedParameterCommandAccessor::GetExposedParam(const xiiDocumentObject* pObject, const char* szParamName)
{
  if (const xiiExposedParameters* pParams = GetExposedParams(pObject))
  {
    return pParams->Find(szParamName);
  }
  return nullptr;
}

const xiiRTTI* xiiExposedParameterCommandAccessor::GetExposedParamsType(const xiiDocumentObject* pObject)
{
  xiiVariant value;
  if (xiiObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<xiiString>())
    {
      const auto& sValue = value.Get<xiiString>();
      if (const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(sValue.GetData()))
      {
        return xiiExposedParametersTypeRegistry::GetSingleton()->GetExposedParametersType(sValue);
      }
    }
  }
  return nullptr;
}

const xiiRTTI* xiiExposedParameterCommandAccessor::GetCommonExposedParamsType(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  const xiiRTTI* type   = nullptr;
  bool           bFirst = true;
  // check if we have multiple values
  for (const auto& item : items)
  {
    if (bFirst)
    {
      type = GetExposedParamsType(item.m_pObject);
    }
    else
    {
      auto type2 = GetExposedParamsType(item.m_pObject);
      if (type != type2)
      {
        return nullptr;
      }
    }
  }
  return type;
}

bool xiiExposedParameterCommandAccessor::IsExposedProperty(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  if (auto type = GetExposedParamsType(pObject))
  {
    auto props = type->GetProperties();
    return std::any_of(cbegin(props), cend(props), [&](const xiiAbstractProperty* pOtherProp) { return pOtherProp == pProp; });
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////

void xiiQtExposedParameterPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  xiiVariantType::Enum commonType = xiiVariantType::Invalid;
  GetCommonVariantSubType(m_Items, m_pProp, commonType);
  const xiiRTTI* pNewtSubType = commonType != xiiVariantType::Invalid ? xiiReflectionUtils::GetTypeFromVariant(commonType) : nullptr;

  xiiExposedParameterCommandAccessor* proxy = static_cast<xiiExposedParameterCommandAccessor*>(m_pObjectAccessor);
  if (auto type = proxy->GetCommonExposedParamsType(m_Items))
  {
    if (auto prop = type->FindPropertyByName(m_Items[0].m_Index.Get<xiiString>()))
    {
      auto paramDefault = xiiToolsReflectionUtils::GetStorageDefault(prop);
      if (paramDefault.GetType() == commonType)
      {
        if (prop->GetSpecificType() != m_pCurrentSubType || m_pWidget == nullptr)
        {
          if (m_pWidget)
          {
            m_pWidget->PrepareToDie();
            m_pWidget->deleteLater();
            m_pWidget = nullptr;
          }
          m_pCurrentSubType = pNewtSubType;
          m_pWidget         = xiiQtPropertyGridWidget::CreateMemberPropertyWidget(prop);
          if (!m_pWidget)
            m_pWidget = new xiiQtUnsupportedPropertyWidget("Unsupported type");

          m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
          m_pWidget->setParent(this);
          m_pLayout->addWidget(m_pWidget);
          m_pWidget->Init(m_pGrid, m_pObjectAccessor, type, prop);

          UpdateTypeListSelection(commonType);
        }
        m_pWidget->SetSelection(m_Items);
        return;
      }
    }
  }
  xiiQtVariantPropertyWidget::InternalSetValue(value);
}

//////////////////////////////////////////////////////////////////////////

xiiQtExposedParametersPropertyWidget::xiiQtExposedParametersPropertyWidget() = default;

xiiQtExposedParametersPropertyWidget::~xiiQtExposedParametersPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
}

void xiiQtExposedParametersPropertyWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  xiiQtPropertyStandardTypeContainerWidget::SetSelection(items);
  UpdateActionState();
}

void xiiQtExposedParametersPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));

  const auto* pAttrib = m_pProp->GetAttributeByType<xiiExposedParametersAttribute>();
  XII_ASSERT_DEV(pAttrib, "xiiQtExposedParametersPropertyWidget was created for a property that does not have the xiiExposedParametersAttribute.");
  m_sExposedParamProperty                         = pAttrib->GetParametersSource();
  const xiiAbstractProperty* pParameterSourceProp = m_pType->FindPropertyByName(m_sExposedParamProperty);
  XII_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_sExposedParamProperty, m_pType->GetTypeName());
  m_pSourceObjectAccessor = m_pObjectAccessor;
  m_pProxy                = XII_DEFAULT_NEW(xiiExposedParameterCommandAccessor, m_pSourceObjectAccessor, m_pProp, pParameterSourceProp);
  m_pObjectAccessor       = m_pProxy.Borrow();

  xiiQtPropertyStandardTypeContainerWidget::OnInit();

  {
    // Help button to indicate exposed parameter mismatches.
    m_pFixMeButton = new QToolButton();
    m_pFixMeButton->setAutoRaise(true);
    m_pFixMeButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    m_pFixMeButton->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/Attention.svg"));
    auto sp = m_pFixMeButton->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    m_pFixMeButton->setSizePolicy(sp);
    QMenu* pFixMeMenu = new QMenu(m_pFixMeButton);
    {
      m_pRemoveUnusedAction = pFixMeMenu->addAction(QStringLiteral("Remove unused keys"));
      m_pRemoveUnusedAction->setToolTip(QStringLiteral("The map contains keys that are no longer used by the asset's exposed parameters and thus can be removed."));
      connect(m_pRemoveUnusedAction, &QAction::triggered, this, [this](bool bChecked) { RemoveUnusedKeys(false); });
    }
    {
      m_pFixTypesAction = pFixMeMenu->addAction(QStringLiteral("Fix keys with wrong types"));
      connect(m_pFixTypesAction, &QAction::triggered, this, [this](bool bChecked) { FixKeyTypes(false); });
    }
    m_pFixMeButton->setMenu(pFixMeMenu);

    auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());
    layout->insertWidget(layout->count() - 1, m_pFixMeButton);
  }
}

xiiQtPropertyWidget* xiiQtExposedParametersPropertyWidget::CreateWidget(xiiUInt32 index)
{
  return new xiiQtExposedParameterPropertyWidget();
}

void xiiQtExposedParametersPropertyWidget::UpdateElement(xiiUInt32 index)
{
  xiiQtPropertyStandardTypeContainerWidget::UpdateElement(index);
}

void xiiQtExposedParametersPropertyWidget::UpdatePropertyMetaState()
{
  xiiQtPropertyStandardTypeContainerWidget::UpdatePropertyMetaState();
  return;

  for (xiiUInt32 i = 0; i < m_Elements.GetCount(); i++)
  {
    Element&    elem      = m_Elements[i];
    const auto& selection = elem.m_pWidget->GetSelection();
    bool        isDefault = true;
    for (const auto& item : selection)
    {
      xiiVariant value;
      xiiStatus  res = m_pSourceObjectAccessor->GetValue(item.m_pObject, m_pProp, value, item.m_Index);
      if (res.Succeeded())
      {
        // In case we successfully read the value from the source accessor (not the proxy that pretends all exposed params exist)
        // we now the value is overwritten as in the default case the map index would not exist.
        isDefault = false;
        break;
      }
    }
    elem.m_pWidget->SetIsDefault(isDefault);
    elem.m_pSubGroup->SetBoldTitle(!isDefault);
  }
}

void xiiQtExposedParametersPropertyWidget::PropertyEventHandler(const xiiDocumentObjectPropertyEvent& e)
{
  if (IsUndead())
    return;

  if (std::none_of(cbegin(m_Items), cend(m_Items), [=](const xiiPropertySelection& sel) { return e.m_pObject == sel.m_pObject; }))
    return;

  if (!m_bNeedsUpdate && m_sExposedParamProperty == e.m_sProperty)
  {
    m_bNeedsUpdate = true;
    // In case the change happened outside the command history we have to update at once.
    if (!m_pGrid->GetCommandHistory()->IsInTransaction() && !m_pGrid->GetCommandHistory()->IsInUndoRedo())
      FlushQueuedChanges();
  }
  if (!m_bNeedsMetaDataUpdate && m_pProp->GetPropertyName() == e.m_sProperty)
  {
    m_bNeedsMetaDataUpdate = true;
    if (!m_pGrid->GetCommandHistory()->IsInTransaction() && !m_pGrid->GetCommandHistory()->IsInUndoRedo())
      FlushQueuedChanges();
  }
}

void xiiQtExposedParametersPropertyWidget::CommandHistoryEventHandler(const xiiCommandHistoryEvent& e)
{
  if (IsUndead())
    return;

  switch (e.m_Type)
  {
    case xiiCommandHistoryEvent::Type::UndoEnded:
    case xiiCommandHistoryEvent::Type::RedoEnded:
    case xiiCommandHistoryEvent::Type::TransactionEnded:
    case xiiCommandHistoryEvent::Type::TransactionCanceled:
    {
      FlushQueuedChanges();
    }
    break;

    default:
      break;
  }
}

void xiiQtExposedParametersPropertyWidget::FlushQueuedChanges()
{
  if (m_bNeedsUpdate)
  {
    m_bNeedsUpdate = false;
    SetSelection(m_Items);
  }
  if (m_bNeedsMetaDataUpdate)
  {
    UpdateActionState();
  }
}

bool xiiQtExposedParametersPropertyWidget::RemoveUnusedKeys(bool bTestOnly)
{
  bool bStuffDone = false;
  if (!bTestOnly)
    m_pSourceObjectAccessor->StartTransaction("Remove unused keys");
  for (const auto& item : m_Items)
  {
    if (const xiiExposedParameters* pParams = m_pProxy->GetExposedParams(item.m_pObject))
    {
      xiiHybridArray<xiiVariant, 16> keys;
      XII_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (!pParams->Find(key.Get<xiiString>()))
        {
          if (!bTestOnly)
          {
            bStuffDone = true;
            m_pSourceObjectAccessor->RemoveValue(item.m_pObject, m_pProp, key).LogFailure();
          }
          else
          {
            return true;
          }
        }
      }
    }
  }
  if (!bTestOnly)
    m_pSourceObjectAccessor->FinishTransaction();
  return bStuffDone;
}

bool xiiQtExposedParametersPropertyWidget::FixKeyTypes(bool bTestOnly)
{
  bool bStuffDone = false;
  if (!bTestOnly)
    m_pSourceObjectAccessor->StartTransaction("Remove unused keys");
  for (const auto& item : m_Items)
  {
    if (const xiiExposedParameters* pParams = m_pProxy->GetExposedParams(item.m_pObject))
    {
      xiiHybridArray<xiiVariant, 16> keys;
      XII_VERIFY(m_pSourceObjectAccessor->GetKeys(item.m_pObject, m_pProp, keys).Succeeded(), "");
      for (auto& key : keys)
      {
        if (const auto* pParam = pParams->Find(key.Get<xiiString>()))
        {
          xiiVariant     value;
          const xiiRTTI* pType = pParam->m_DefaultValue.GetReflectedType();
          XII_VERIFY(m_pSourceObjectAccessor->GetValue(item.m_pObject, m_pProp, value, key).Succeeded(), "");
          if (value.GetReflectedType() != pType)
          {
            if (!bTestOnly)
            {
              bStuffDone                = true;
              xiiVariantType::Enum type = pParam->m_DefaultValue.GetType();
              if (value.CanConvertTo(type))
              {
                m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), key).LogFailure();
              }
              else
              {
                m_pObjectAccessor->SetValue(item.m_pObject, m_pProp, pParam->m_DefaultValue, key).LogFailure();
              }
            }
            else
            {
              return true;
            }
          }
        }
      }
    }
  }
  if (!bTestOnly)
    m_pSourceObjectAccessor->FinishTransaction();
  return bStuffDone;
}

void xiiQtExposedParametersPropertyWidget::UpdateActionState()
{
  m_bNeedsMetaDataUpdate = false;
  m_pRemoveUnusedAction->setEnabled(RemoveUnusedKeys(true));
  m_pFixTypesAction->setEnabled(FixKeyTypes(true));
  m_pFixMeButton->setVisible(m_pRemoveUnusedAction->isEnabled() || m_pFixTypesAction->isEnabled());
}
