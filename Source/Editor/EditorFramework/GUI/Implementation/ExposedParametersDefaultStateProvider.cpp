#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

xiiSharedPtr<xiiDefaultStateProvider> xiiExposedParametersDefaultStateProvider::CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  if (pProp)
  {
    const auto* pAttrib = pProp->GetAttributeByType<xiiExposedParametersAttribute>();
    if (pAttrib)
    {
      return XII_DEFAULT_NEW(xiiExposedParametersDefaultStateProvider, pAccessor, pObject, pProp);
    }
  }
  return nullptr;
}

xiiExposedParametersDefaultStateProvider::xiiExposedParametersDefaultStateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) :
  m_pObject(pObject), m_pProp(pProp)
{
  XII_ASSERT_DEBUG(pProp->GetCategory() == xiiPropertyCategory::Map, "xiiExposedParametersAttribute must be on a map property");
  m_pAttrib = pProp->GetAttributeByType<xiiExposedParametersAttribute>();
  XII_ASSERT_DEBUG(m_pAttrib, "xiiExposedParametersDefaultStateProvider was created for a property that does not have the xiiExposedParametersAttribute.");
  m_pParameterSourceProp = pObject->GetType()->FindPropertyByName(m_pAttrib->GetParametersSource());
  XII_ASSERT_DEBUG(m_pParameterSourceProp, "The exposed parameter source '{0}' does not exist on type '{1}'", m_pAttrib->GetParametersSource(), pObject->GetType()->GetTypeName());
}

xiiInt32 xiiExposedParametersDefaultStateProvider::GetRootDepth() const
{
  return 0;
}

xiiColorGammaUB xiiExposedParametersDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return xiiColorGammaUB(0, 0, 0, 0);
}

xiiVariant xiiExposedParametersDefaultStateProvider::GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  XII_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "xiiDefaultContainerState is only valid on the object and container it was created on.");
  xiiExposedParameterCommandAccessor accessor(pAccessor, pProp, m_pParameterSourceProp);
  if (index.IsValid())
  {
    if (index.IsA<xiiString>())
    {
      const xiiExposedParameter* pParam = accessor.GetExposedParam(pObject, index.Get<xiiString>());
      if (pParam)
      {
        return pParam->m_DefaultValue;
      }
    }
    return superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), pAccessor, pObject, pProp, index);
  }
  else
  {
    xiiVariantDictionary defaultDict;
    if (const xiiExposedParameters* pParams = accessor.GetExposedParams(pObject))
    {
      for (xiiExposedParameter* pParam : pParams->m_Parameters)
      {
        defaultDict.Insert(pParam->m_sName, pParam->m_DefaultValue);
      }
    }
    return defaultDict;
  }
}

xiiStatus xiiExposedParametersDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff)
{
  XII_REPORT_FAILURE("Unreachable code");
  return XII_SUCCESS;
}

bool xiiExposedParametersDefaultStateProvider::IsDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  XII_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "xiiDefaultContainerState is only valid on the object and container it was created on.");
  xiiExposedParameterCommandAccessor accessor(pAccessor, pProp, m_pParameterSourceProp);

  const xiiVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
  if (index.IsValid())
  {
    xiiVariant value;
    xiiStatus  res = accessor.GetValue(pObject, pProp, value, index);
    // If the key is not valid, the exposed parameter is not overwritten and thus remains at the default value.
    return res.Failed() || def == value;
  }
  else
  {
    // We consider an exposed params map to be the default if it is empty.
    // We deliberately do not use the accessor here and go directly to the object storage as the passed in pAccessor could already be a xiiExposedParameterCommandAccessor in which case we wouldn't truly know if anything was overwritten.
    xiiVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index);
    return value.Get<xiiVariantDictionary>().GetCount() == 0;
  }
}

xiiStatus xiiExposedParametersDefaultStateProvider::RevertProperty(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  if (!index.IsValid())
  {
    // We override the standard implementation here to just clear the array on revert to default. This is because the exposed params work as an override of the default behavior and we can safe space and time by simply not overriding anything.
    // The GUI will take care of pretending that the values are present with their default value.
    xiiDeque<xiiAbstractGraphDiffOperation> diff;
    auto&                                   op = diff.ExpandAndGetRef();
    op.m_Node                                  = pObject->GetGuid();
    op.m_Operation                             = xiiAbstractGraphDiffOperation::Op::PropertyChanged;
    op.m_sProperty                             = pProp->GetPropertyName();
    op.m_uiTypeVersion                         = 0;
    op.m_Value                                 = xiiVariantDictionary();
    xiiDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
    return XII_SUCCESS;
  }
  return xiiDefaultStateProvider::RevertProperty(superPtr, pAccessor, pObject, pProp, index);
}

///////////////////////////////////////////////////////////////////////////////

xiiSharedPtr<xiiDefaultStateProvider> xiiExposedParametersAsTypeDefaultStateProvider::CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  if (auto pExposedParameterCommandAccessor = xiiDynamicCast<xiiExposedParametersAsTypeCommandAccessor*>(pAccessor))
  {
    return XII_DEFAULT_NEW(xiiExposedParametersAsTypeDefaultStateProvider, pExposedParameterCommandAccessor, pObject, pProp);
  }
  return nullptr;
}

xiiExposedParametersAsTypeDefaultStateProvider::xiiExposedParametersAsTypeDefaultStateProvider(xiiExposedParametersAsTypeCommandAccessor* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) :
  xiiExposedParametersDefaultStateProvider(pAccessor->GetSourceAccessor(), pObject, pAccessor->GetSourceAccessor()->m_pParameterProp), m_pAccessor(pAccessor)
{
}

xiiVariant xiiExposedParametersAsTypeDefaultStateProvider::GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  xiiVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Succeeded())
    return defaultValue;

  return {};
}

xiiStatus xiiExposedParametersAsTypeDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff)
{
  XII_REPORT_FAILURE("Unreachable code");
  return xiiStatus(XII_SUCCESS);
}

bool xiiExposedParametersAsTypeDefaultStateProvider::IsDefaultValue(xiiDefaultStateProvider::SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  xiiVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return true;

  xiiVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();
  return defaultValue == value;
}

xiiStatus xiiExposedParametersAsTypeDefaultStateProvider::RevertProperty(xiiDefaultStateProvider::SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  xiiVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return xiiStatus(xiiFmt("Failed to retrieve default value for exposed parameter."));

  return pAccessor->SetValue(pObject, pProp, defaultValue, index);
}

xiiResult xiiExposedParametersAsTypeDefaultStateProvider::GetDefaultValueInternal(xiiDefaultStateProvider::SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiVariant& out_DefaultValue)
{
  // As we derive from xiiExposedParametersDefaultStateProvider, we first need to convert the exposed parameter type, prop, accessor into the actual underlying data structure that the base class expects before calling it.
  // * The xiiExposedParametersAsTypeCommandAccessor proxies the xiiExposedParameterCommandAccessor so the proxy source is the correct accessor.
  // * The object stays the same.
  // * The property from the exposed parameter type is replaced by the actual property that stores the exposed parameters in the real object.
  // * The index is the name of the property as that is how the parameter map is generated (keyed by parameter name).
  // With these changes made, we can rely on the base class to compute the default value.
  out_DefaultValue = xiiExposedParametersDefaultStateProvider::GetDefaultValue(superPtr.GetSubArray(1), m_pAccessor->GetSourceAccessor(), pObject, m_pAccessor->GetSourceAccessor()->m_pParameterProp, pProp->GetPropertyName());

  xiiStatus res(XII_SUCCESS);
  // We now have the value of the exposed parameter. If this is a container, we need to dive into the index. If index is invalid, this is a no-op.
  out_DefaultValue = xiiVariantStorageAccessor(pProp->GetPropertyName(), out_DefaultValue).GetValue(index, &res);
  if (res.Failed())
    return XII_FAILURE;

  return XII_SUCCESS;
}
