#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParametersDefaultStateProvider.h>
#include <EditorFramework/PropertyGrid/ExposedParametersPropertyWidget.moc.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
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
    const xiiExposedParameter* pParam = accessor.GetExposedParam(pObject, index.Get<xiiString>());
    if (pParam)
    {
      return pParam->m_DefaultValue;
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
  return xiiStatus(XII_SUCCESS);
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
    return xiiStatus(XII_SUCCESS);
  }
  return xiiDefaultStateProvider::RevertProperty(superPtr, pAccessor, pObject, pProp, index);
}
