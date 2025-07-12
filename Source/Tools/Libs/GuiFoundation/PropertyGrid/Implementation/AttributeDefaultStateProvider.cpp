#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/AttributeDefaultStateProvider.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

static xiiSharedPtr<xiiDefaultStateProvider> g_pAttributeDefaultStateProvider = XII_DEFAULT_NEW(xiiAttributeDefaultStateProvider);
xiiSharedPtr<xiiDefaultStateProvider>        xiiAttributeDefaultStateProvider::CreateProvider(xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  // One global instance handles all. No need to create a new instance per request as no state need to be tracked.
  return g_pAttributeDefaultStateProvider;
}

xiiInt32 xiiAttributeDefaultStateProvider::GetRootDepth() const
{
  return -1;
}

xiiColorGammaUB xiiAttributeDefaultStateProvider::GetBackgroundColor() const
{
  // Set alpha to 0 -> color will be ignored.
  return xiiColorGammaUB(0, 0, 0, 0);
}

xiiVariant xiiAttributeDefaultStateProvider::GetDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  if (!pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) && pProp->GetFlags().IsSet(xiiPropertyFlags::Class) && pProp->GetCategory() == xiiPropertyCategory::Member && !xiiReflectionUtils::IsValueType(pProp))
  {
    // An embedded class that is not a value type can never change its value.
    xiiVariant value;
    pAccessor->GetValue(pObject, pProp, value).LogFailure();
    return value;
  }
  return xiiReflectionUtils::GetDefaultValue(pProp, index);
}

xiiStatus xiiAttributeDefaultStateProvider::CreateRevertContainerDiff(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiDeque<xiiAbstractGraphDiffOperation>& out_diff)
{
  auto RemoveObject = [&](const xiiUuid& object) {
    auto& op           = out_diff.ExpandAndGetRef();
    op.m_Node          = object;
    op.m_Operation     = xiiAbstractGraphDiffOperation::Op::NodeRemoved;
    op.m_uiTypeVersion = 0;
    op.m_sProperty     = pAccessor->GetObject(object)->GetType()->GetTypeName();
  };

  auto SetProperty = [&](const xiiVariant& newValue) {
    auto& op           = out_diff.ExpandAndGetRef();
    op.m_Node          = pObject->GetGuid();
    op.m_Operation     = xiiAbstractGraphDiffOperation::Op::PropertyChanged;
    op.m_uiTypeVersion = 0;
    op.m_sProperty     = pProp->GetPropertyName();
    op.m_Value         = newValue;
  };

  xiiVariant currentValue;
  pAccessor->GetValue(pObject, pProp, currentValue).LogFailure();
  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      const auto& objectGuid = currentValue.Get<xiiUuid>();
      if (objectGuid.IsValid())
      {
        RemoveObject(objectGuid);
        SetProperty(xiiUuid());
      }
    }
    break;
    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Set:
    {
      const auto& currentArray = currentValue.Get<xiiVariantArray>();
      for (xiiInt32 i = (xiiInt32)currentArray.GetCount() - 1; i >= 0; i--)
      {
        const auto& objectGuid = currentArray[i].Get<xiiUuid>();
        if (objectGuid.IsValid())
        {
          RemoveObject(objectGuid);
        }
      }
      SetProperty(xiiVariantArray());
    }
    break;
    case xiiPropertyCategory::Map:
    {
      const auto& currentArray = currentValue.Get<xiiVariantDictionary>();
      for (auto val : currentArray)
      {
        const auto& objectGuid = val.Value().Get<xiiUuid>();
        if (objectGuid.IsValid())
        {
          RemoveObject(objectGuid);
        }
      }
      SetProperty(xiiVariantDictionary());
    }
    break;
    default:
      XII_REPORT_FAILURE("Unreachable code");
      break;
  }
  return XII_SUCCESS;
}
