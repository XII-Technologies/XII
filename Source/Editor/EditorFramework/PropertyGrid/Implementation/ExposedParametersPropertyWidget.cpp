#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/GUI/ExposedParametersTypeRegistry.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/GroupBoxBase.moc.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposedParameterCommandAccessor, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiExposedParametersAsTypeCommandAccessor, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

bool xiiQtExposedParametersPropertyWidget::s_bRawMode = false;

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
  else if (res.Failed() && m_pParameterProp == pProp && (index.IsA<xiiString>() || index.IsA<xiiStringView>()))
  {
    // If the actual GetValue fails but the key is an exposed param, return its default value instead.
    if (const xiiExposedParameter* pParam = GetExposedParam(pObject, index.ConvertTo<xiiString>()))
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
  // As we pretend the exposed params always exist the actual SetValue will fail if this is not actually true, so we redirect to insert to make it true.
  if (res.Failed() && m_pParameterProp == pProp && (index.IsA<xiiString>() || index.IsA<xiiStringView>()))
  {
    return xiiExposedParameterCommandAccessor::InsertValue(pObject, pProp, newValue, index.ConvertTo(xiiVariantType::String));
  }
  return res;
}

xiiStatus xiiExposedParameterCommandAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index /*= xiiVariant()*/)
{
  xiiStatus res = xiiObjectProxyAccessor::RemoveValue(pObject, pProp, index);
  if (res.Failed() && m_pParameterProp == pProp && (index.IsA<xiiString>() || index.IsA<xiiStringView>()))
  {
    // It this is one of the exposed params, pretend we removed it successfully to suppress error messages.
    if (const xiiExposedParameter* pParam = GetExposedParam(pObject, index.ConvertTo<xiiString>()))
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
    for (const xiiVariant& key : keys)
    {
      xiiVariant& var = out_values.ExpandAndGetRef();
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
    if (value.IsA<xiiString>() || value.IsA<xiiStringView>())
    {
      const xiiString& sValue = value.ConvertTo<xiiString>();
      if (const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(sValue))
      {
        return asset->m_pAssetInfo->m_Info->GetMetaInfo<xiiExposedParameters>();
      }
    }
  }
  return nullptr;
}

const xiiExposedParameter* xiiExposedParameterCommandAccessor::GetExposedParam(const xiiDocumentObject* pObject, xiiStringView sParamName)
{
  if (const xiiExposedParameters* pParams = GetExposedParams(pObject))
  {
    return pParams->Find(sParamName);
  }
  return nullptr;
}

const xiiRTTI* xiiExposedParameterCommandAccessor::GetExposedParamsType(const xiiDocumentObject* pObject)
{
  xiiVariant value;
  if (xiiObjectProxyAccessor::GetValue(pObject, m_pParameterSourceProp, value).Succeeded())
  {
    if (value.IsA<xiiString>() || value.IsA<xiiStringView>())
    {
      const xiiString& sValue = value.ConvertTo<xiiString>();
      if (const auto asset = xiiAssetCurator::GetSingleton()->FindSubAsset(sValue))
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
      type   = GetExposedParamsType(item.m_pObject);
      bFirst = false;
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

xiiExposedParametersAsTypeCommandAccessor::xiiExposedParametersAsTypeCommandAccessor(xiiExposedParameterCommandAccessor* pSource) :
  xiiObjectProxyAccessor(pSource)
{
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index)
{
  XII_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, out_value));

  xiiStatus result(XII_SUCCESS);
  out_value = xiiVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(index, &result);
  return result;
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index)
{
  return SetSubValue(pObject, pProp, [&](xiiVariant& subValue) -> xiiStatus { return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).SetValue(newValue, index); });
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index)
{
  return SetSubValue(pObject, pProp, [&](xiiVariant& subValue) -> xiiStatus { return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).InsertValue(index, newValue); });
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  return SetSubValue(pObject, pProp, [&](xiiVariant& subValue) -> xiiStatus { return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).RemoveValue(index); });
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  return SetSubValue(pObject, pProp, [&](xiiVariant& subValue) -> xiiStatus { return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).MoveValue(oldIndex, newIndex); });
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount)
{
  xiiVariant subValue;
  XII_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  out_iCount = xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetCount();
  return XII_SUCCESS;
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys)
{
  xiiVariant subValue;
  XII_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetKeys(out_keys);
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values)
{
  xiiVariant subValue;
  XII_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  xiiHybridArray<xiiVariant, 16> keys;
  xiiVariantStorageAccessor      accessor(pProp->GetPropertyName(), subValue);
  XII_SUCCEED_OR_RETURN(accessor.GetKeys(keys));
  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (const xiiVariant& key : keys)
  {
    out_values.PushBack(accessor.GetValue(key));
  }
  return XII_SUCCESS;
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::GetSubValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value)
{
  const xiiRTTI* pType = GetSourceAccessor()->GetExposedParamsType(pObject);
  XII_ASSERT_DEBUG(pType && pType->FindPropertyByName(pProp->GetPropertyName()) == pProp, "");

  xiiStatus result = GetSourceAccessor()->GetValue(pObject, GetSourceAccessor()->m_pParameterProp, out_value, pProp->GetPropertyName());
  if (result.Failed())
    return result;

  PatchPropertyType(out_value, pProp);

  return result;
}

xiiStatus xiiExposedParametersAsTypeCommandAccessor::SetSubValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiDelegate<xiiStatus(xiiVariant&)>& func)
{
  xiiVariant currentValue;
  XII_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, currentValue));
  XII_SUCCEED_OR_RETURN(func(currentValue));
  return GetSourceAccessor()->SetValue(pObject, pProp, currentValue, pProp->GetPropertyName());
}

void xiiExposedParametersAsTypeCommandAccessor::PatchPropertyType(xiiVariant& ref_value, const xiiAbstractProperty* pProp)
{
  const xiiVariantType::Enum propType  = xiiToolsReflectionUtils::GetStorageType(pProp);
  const xiiVariantType::Enum valueType = ref_value.GetType();
  if (propType != valueType)
  {
    if (ref_value.CanConvertTo(propType))
    {
      ref_value = ref_value.ConvertTo(propType);
    }
    else
    {
      ref_value = xiiToolsReflectionUtils::GetStorageDefault(pProp);
    }
  }

  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Array:
      if (const xiiVariantType::Enum propElementType = pProp->GetSpecificType()->GetVariantType(); propElementType != xiiVariantType::Invalid)
      {
        const xiiVariantArray& array = ref_value.Get<xiiVariantArray>();
        for (xiiUInt32 i = 0; i < array.GetCount(); ++i)
        {
          const xiiVariant&          element          = array[i];
          const xiiVariantType::Enum valueElementType = element.GetType();
          if (propElementType != valueElementType)
          {
            xiiVariantArray& arrayWritable   = ref_value.GetWritable<xiiVariantArray>();
            xiiVariant&      elementWritable = arrayWritable[i];
            if (elementWritable.CanConvertTo(propElementType))
            {
              elementWritable = elementWritable.ConvertTo(propType);
            }
            else
            {
              elementWritable = xiiReflectionUtils::GetDefaultVariantFromType(propElementType);
            }
          }
        }
      }
      break;
    case xiiPropertyCategory::Map:
      if (const xiiVariantType::Enum propElementType = pProp->GetSpecificType()->GetVariantType(); propElementType != xiiVariantType::Invalid)
      {
        const xiiVariantDictionary& map = ref_value.Get<xiiVariantDictionary>();
        for (auto it : map)
        {
          const xiiVariant&          element          = it.Value();
          const xiiVariantType::Enum valueElementType = element.GetType();
          if (propElementType != valueElementType)
          {
            xiiVariantDictionary& mapWritable      = ref_value.GetWritable<xiiVariantDictionary>();
            xiiVariant*           pElementWritable = nullptr;
            mapWritable.TryGetValue(it.Key(), pElementWritable);
            if (pElementWritable->CanConvertTo(propElementType))
            {
              *pElementWritable = pElementWritable->ConvertTo(propType);
            }
            else
            {
              *pElementWritable = xiiReflectionUtils::GetDefaultVariantFromType(propElementType);
            }
          }
        }
      }
      break;

    default:
      break;
  }
}

//////////////////////////////////////////////////////////////////////////

xiiQtExposedParametersPropertyWidget::xiiQtExposedParametersPropertyWidget() :
  xiiQtPropertyStandardTypeContainerWidget()
{
  // Replace the container layout so we can prepend the type widget before all container elements
  delete m_pGroupLayout;
  m_pGroupLayout = new QVBoxLayout(nullptr);
  m_pGroupLayout->setSpacing(1);
  m_pGroupLayout->setContentsMargins(5, 0, 0, 0);

  m_pTypeViewLayout = new QVBoxLayout(nullptr);
  m_pTypeViewLayout->addLayout(m_pGroupLayout);
  m_pTypeViewLayout->setSpacing(0);
  m_pTypeViewLayout->setContentsMargins(0, 0, 0, 0);

  m_pGroup->GetContent()->setLayout(m_pTypeViewLayout);
}

xiiQtExposedParametersPropertyWidget::~xiiQtExposedParametersPropertyWidget()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
  xiiPhantomRttiManager::s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler, this));
}

void xiiQtExposedParametersPropertyWidget::SetSelection(const xiiHybridArray<xiiPropertySelection, 8>& items)
{
  const xiiRTTI* pCommonType = m_pProxy->GetCommonExposedParamsType(items);
  if (m_pTypeWidget && m_pTypeWidget->GetType() != pCommonType)
  {
    m_pTypeWidget->PrepareToDie();
    m_pTypeWidget->deleteLater();
    m_pTypeWidget = nullptr;
  }

  if (m_pTypeWidget == nullptr && pCommonType != nullptr)
  {
    m_pTypeWidget = new xiiQtTypeWidget(m_pGroup->GetContent(), m_pGrid, m_pTypeProxy.Borrow(), pCommonType, nullptr, nullptr);
    m_pTypeViewLayout->insertWidget(0, m_pTypeWidget);
  }

  if (m_pTypeWidget)
  {
    m_pTypeWidget->setVisible(!s_bRawMode);
    m_pTypeWidget->SetSelection(items);
  }


  xiiQtPropertyStandardTypeContainerWidget::SetSelection(items);
  UpdateActionState();
}

void xiiQtExposedParametersPropertyWidget::OnInit()
{
  m_pGrid->GetObjectManager()->m_PropertyEvents.AddEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::PropertyEventHandler, this));
  m_pGrid->GetCommandHistory()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::CommandHistoryEventHandler, this));
  xiiPhantomRttiManager::s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler, this));

  const auto* pAttrib = m_pProp->GetAttributeByType<xiiExposedParametersAttribute>();
  XII_ASSERT_DEV(pAttrib, "xiiQtExposedParametersPropertyWidget was created for a property that does not have the xiiExposedParametersAttribute.");
  m_sExposedParamProperty                         = pAttrib->GetParametersSource();
  const xiiAbstractProperty* pParameterSourceProp = m_pType->FindPropertyByName(m_sExposedParamProperty);
  XII_ASSERT_DEV(pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_sExposedParamProperty, m_pType->GetTypeName());
  m_pSourceObjectAccessor = m_pObjectAccessor;
  m_pProxy                = XII_DEFAULT_NEW(xiiExposedParameterCommandAccessor, m_pSourceObjectAccessor, m_pProp, pParameterSourceProp);
  m_pTypeProxy            = XII_DEFAULT_NEW(xiiExposedParametersAsTypeCommandAccessor, m_pProxy.Borrow());
  // Overwriting this will display the exposed parameter map as before, i.e. each property will be shown in the map even if not present. As this is now obsolete given the phantom type widget, this is probably no longer needed?
  // m_pObjectAccessor = m_pProxy.Borrow();

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
      m_pRemoveUnusedAction->setToolTip(
        QStringLiteral("The map contains keys that are no longer used by the asset's exposed parameters and thus can be removed."));
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

  {
    // Button to toggle between type-based view and the raw dictionary-based view of the exposed parameters.
    m_pToggleRawModeButton = new QToolButton();
    m_pToggleRawModeButton->setAutoRaise(true);
    m_pToggleRawModeButton->setCheckable(true);
    m_pToggleRawModeButton->setChecked(s_bRawMode);
    m_pToggleRawModeButton->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/ExposedParameterViewToggle.svg"));
    auto sp = m_pToggleRawModeButton->sizePolicy();
    sp.setVerticalPolicy(QSizePolicy::Ignored);
    m_pToggleRawModeButton->setSizePolicy(sp);
    m_pToggleRawModeButton->setToolTip("Toggle between Type or RAW dictionary view");

    connect(m_pToggleRawModeButton, &QToolButton::toggled, this, [this](bool checked) {
      s_bRawMode = checked;
      xiiQtScopedUpdatesDisabled _(this);
      SetSelection(m_Items);
    });

    auto layout = qobject_cast<QHBoxLayout*>(m_pGroup->GetHeader()->layout());
    layout->insertWidget(layout->count() - 1, m_pToggleRawModeButton);
  }
}

void xiiQtExposedParametersPropertyWidget::UpdateElement(xiiUInt32 index)
{
  xiiQtPropertyStandardTypeContainerWidget::UpdateElement(index);
}

void xiiQtExposedParametersPropertyWidget::UpdatePropertyMetaState()
{
  xiiQtPropertyStandardTypeContainerWidget::UpdatePropertyMetaState();
}

void xiiQtExposedParametersPropertyWidget::GetRequiredElements(xiiDynamicArray<xiiVariant>& out_keys) const
{
  if (!s_bRawMode)
  {
    out_keys.Clear();
  }
  else
  {
    xiiQtPropertyContainerWidget::GetRequiredElements(out_keys);
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
    FlushOrQueueChanges(true, false);
  }
  if (!m_bNeedsMetaDataUpdate && m_pProp->GetPropertyName() == e.m_sProperty)
  {
    FlushOrQueueChanges(false, true);
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
      FlushOrQueueChanges(false, false);
    }
    break;

    default:
      break;
  }
}

void xiiQtExposedParametersPropertyWidget::PhantomTypeRegistryEventHandler(const xiiPhantomRttiManagerEvent& e)
{
  if (const xiiRTTI* pCommonType = m_pProxy->GetCommonExposedParamsType(m_Items))
  {
    if (e.m_pChangedType->IsDerivedFrom(pCommonType))
    {
      // The type widget stores pointer to properties which have been destroyed by the phantom type update so we need to destroy it and recreate it.
      m_pTypeWidget->PrepareToDie();
      m_pTypeWidget->deleteLater();
      m_pTypeWidget = nullptr;
      FlushOrQueueChanges(true, true);
    }
  }
}

void xiiQtExposedParametersPropertyWidget::FlushOrQueueChanges(bool bNeedsUpdate, bool bNeedsMetaDataUpdate)
{
  m_bNeedsUpdate |= bNeedsUpdate;
  m_bNeedsMetaDataUpdate |= bNeedsMetaDataUpdate;
  // Wait until the transaction is done and this function will be called again inside CommandHistoryEventHandler.
  if (m_pGrid->GetCommandHistory()->IsInTransaction() || m_pGrid->GetCommandHistory()->IsInUndoRedo())
    return;

  if (m_bNeedsUpdate)
  {
    m_bNeedsUpdate = false;
    SetSelection(m_Items);
  }
  if (m_bNeedsMetaDataUpdate)
  {
    // m_bNeedsMetaDataUpdate is reset inside UpdateActionState as it can be called from other places.
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
                m_pProxy->SetValue(item.m_pObject, m_pProp, value.ConvertTo(type), key).LogFailure();
              }
              else
              {
                m_pProxy->SetValue(item.m_pObject, m_pProp, pParam->m_DefaultValue, key).LogFailure();
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
