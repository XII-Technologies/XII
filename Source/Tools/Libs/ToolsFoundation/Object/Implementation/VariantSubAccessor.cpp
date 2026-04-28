/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiVariantSubAccessor, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiVariantSubAccessor::xiiVariantSubAccessor(xiiObjectAccessorBase* pSource, const xiiAbstractProperty* pProp) :
  xiiObjectProxyAccessor(pSource), m_pProp(pProp)
{
}

void xiiVariantSubAccessor::SetSubItems(const xiiMap<const xiiDocumentObject*, xiiVariant>& subItemMap)
{
  m_SubItemMap = subItemMap;
}

xiiInt32 xiiVariantSubAccessor::GetDepth() const
{
  if (auto variantSubAccessor = xiiDynamicCast<xiiVariantSubAccessor*>(GetSourceAccessor()))
  {
    return variantSubAccessor->GetDepth() + 1;
  }
  return 1;
}

xiiResult xiiVariantSubAccessor::GetPath(const xiiDocumentObject* pObject, xiiDynamicArray<xiiVariant>& out_path) const
{
  out_path.Clear();
  if (auto variantSubAccessor = xiiDynamicCast<xiiVariantSubAccessor*>(GetSourceAccessor()))
  {
    XII_SUCCEED_OR_RETURN(variantSubAccessor->GetPath(pObject, out_path));
  }
  xiiVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return XII_FAILURE;

  out_path.PushBack(subItem);
  return XII_SUCCESS;
}

xiiStatus xiiVariantSubAccessor::GetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value, xiiVariant index)
{
  XII_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, out_value));

  xiiStatus result(XII_SUCCESS);
  out_value = xiiVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(index, &result);
  return result;
}

xiiStatus xiiVariantSubAccessor::SetValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index)
{
  return SetSubValue(pObject, pProp, [&](xiiVariant& subValue) -> xiiStatus { return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).SetValue(newValue, index); });
}

xiiStatus xiiVariantSubAccessor::InsertValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& newValue, xiiVariant index)
{
  return SetSubValue(pObject, pProp, [&](xiiVariant& subValue) -> xiiStatus { return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).InsertValue(index, newValue); });
}

xiiStatus xiiVariantSubAccessor::RemoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  return SetSubValue(pObject, pProp, [&](xiiVariant& subValue) -> xiiStatus { return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).RemoveValue(index); });
}

xiiStatus xiiVariantSubAccessor::MoveValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiVariant& oldIndex, const xiiVariant& newIndex)
{
  return SetSubValue(pObject, pProp, [&](xiiVariant& subValue) -> xiiStatus { return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).MoveValue(oldIndex, newIndex); });
}

xiiStatus xiiVariantSubAccessor::GetCount(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiInt32& out_iCount)
{
  xiiVariant subValue;
  XII_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  out_iCount = xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetCount();
  return XII_SUCCESS;
}

xiiStatus xiiVariantSubAccessor::GetKeys(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_keys)
{
  xiiVariant subValue;
  XII_SUCCEED_OR_RETURN(GetSubValue(pObject, pProp, subValue));
  return xiiVariantStorageAccessor(pProp->GetPropertyName(), subValue).GetKeys(out_keys);
}

xiiStatus xiiVariantSubAccessor::GetValues(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDynamicArray<xiiVariant>& out_values)
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

xiiStatus xiiVariantSubAccessor::GetSubValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant& out_value)
{
  xiiStatus result = xiiObjectProxyAccessor::GetValue(pObject, pProp, out_value);
  if (result.Failed())
    return result;

  xiiVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return xiiStatus(xiiFmt("Sub-item '{0}' not found in variant property '{1}'", subItem, pProp->GetPropertyName()));

  out_value = xiiVariantStorageAccessor(pProp->GetPropertyName(), out_value).GetValue(subItem, &result);
  return result;
}

xiiStatus xiiVariantSubAccessor::SetSubValue(const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, const xiiDelegate<xiiStatus(xiiVariant&)>& func)
{
  XII_ASSERT_DEBUG(m_pProp == pProp, "xiiVariantSubAccessor should only be used to access a single variant property");
  xiiVariant subItem;
  if (!m_SubItemMap.TryGetValue(pObject, subItem))
    return xiiStatus(xiiFmt("Sub-item '{0}' not found in variant property '{1}'", subItem, pProp->GetPropertyName()));

  xiiVariant currentValue;
  XII_SUCCEED_OR_RETURN(xiiObjectProxyAccessor::GetValue(pObject, pProp, currentValue, subItem));
  XII_SUCCEED_OR_RETURN(func(currentValue));
  return xiiObjectProxyAccessor::SetValue(pObject, pProp, currentValue, subItem);
}
