#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/VariantSubDefaultStateProvider.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

xiiSharedPtr<xiiDefaultStateProvider> xiiVariantSubDefaultStateProvider::CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  if (auto variantSubAccessor = xiiDynamicCast<xiiVariantSubAccessor*>(pAccessor))
  {
    if (variantSubAccessor->GetRootProperty() == pProp)
      return XII_DEFAULT_NEW(xiiVariantSubDefaultStateProvider, variantSubAccessor, pObject, pProp);
  }
  return nullptr;
}

xiiVariantSubDefaultStateProvider::xiiVariantSubDefaultStateProvider(xiiVariantSubAccessor* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp) :
  m_pAccessor(pAccessor), m_pObject(pObject), m_pProp(pProp)
{
  m_pRootAccessor = m_pAccessor->GetSourceAccessor();
  while (auto variantSubAccessor = xiiDynamicCast<xiiVariantSubAccessor*>(m_pRootAccessor))
  {
    m_pRootAccessor = variantSubAccessor->GetSourceAccessor();
  }
}

xiiInt32 xiiVariantSubDefaultStateProvider::GetRootDepth() const
{
  // As this default provider dives into the contents of a variable it has to always be executed first as all the other providers work on property granularity.
  return 1000;
}

xiiColorGammaUB xiiVariantSubDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return xiiColorGammaUB(0, 0, 0, 0);
}

xiiVariant xiiVariantSubDefaultStateProvider::GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  xiiVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Succeeded())
    return defaultValue;

  return {};
}

xiiStatus xiiVariantSubDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff)
{
  XII_REPORT_FAILURE("Unreachable code");
  return xiiStatus(XII_SUCCESS);
}

bool xiiVariantSubDefaultStateProvider::IsDefaultValue(xiiDefaultStateProvider::SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  xiiVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return true;

  xiiVariant value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();
  return defaultValue == value;
}

xiiStatus xiiVariantSubDefaultStateProvider::RevertProperty(xiiDefaultStateProvider::SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  xiiVariant defaultValue;
  if (GetDefaultValueInternal(superPtr, pAccessor, pObject, pProp, index, defaultValue).Failed())
    return xiiStatus(xiiFmt("Failed to retrieve default value for variant sub tree."));

  return pAccessor->SetValue(pObject, pProp, defaultValue, index);
}

xiiResult xiiVariantSubDefaultStateProvider::GetDefaultValueInternal(xiiDefaultStateProvider::SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index, xiiVariant& out_DefaultValue)
{
  XII_ASSERT_DEBUG(pObject == m_pObject && pProp == m_pProp, "xiiVariantSubDefaultStateProvider is only valid on the object and variant property it was created on.");
  // As m_pAccessor is a view into an xiiVariant we first need to take the same steps into the defaultValue retrieved from the root accessor to have the same view so we can compare the same subset of both xiiVariants.
  out_DefaultValue = superPtr[0]->GetDefaultValue(superPtr.GetSubArray(1), m_pRootAccessor, pObject, pProp);

  xiiHybridArray<xiiVariant, 4> path;
  XII_SUCCEED_OR_RETURN(m_pAccessor->GetPath(pObject, path));
  if (index.IsValid())
    path.PushBack(index);

  for (const xiiVariant& step : path)
  {
    xiiStatus res(XII_SUCCESS);
    out_DefaultValue = xiiVariantStorageAccessor(pProp->GetPropertyName(), out_DefaultValue).GetValue(step, &res);
    if (res.Failed())
      return XII_FAILURE;
  }
  return XII_SUCCESS;
}
