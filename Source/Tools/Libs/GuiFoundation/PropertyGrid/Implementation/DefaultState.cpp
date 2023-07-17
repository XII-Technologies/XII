#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GuiFoundation/PropertyGrid/AttributeDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/DefaultState.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, DefaultState)
  ON_CORESYSTEMS_STARTUP
  {
    xiiDefaultState::RegisterDefaultStateProvider(xiiAttributeDefaultStateProvider::CreateProvider);
    xiiDefaultState::RegisterDefaultStateProvider(xiiPrefabDefaultStateProvider::CreateProvider);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiDefaultState::UnregisterDefaultStateProvider(xiiAttributeDefaultStateProvider::CreateProvider);
    xiiDefaultState::UnregisterDefaultStateProvider(xiiPrefabDefaultStateProvider::CreateProvider);
  }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on


xiiDynamicArray<xiiDefaultState::CreateStateProviderFunc> xiiDefaultState::s_Factories;

void xiiDefaultState::RegisterDefaultStateProvider(CreateStateProviderFunc func)
{
  s_Factories.PushBack(func);
}

void xiiDefaultState::UnregisterDefaultStateProvider(CreateStateProviderFunc func)
{
  s_Factories.RemoveAndCopy(func);
}

//////////////////////////////////////////////////////////////////////////

xiiDefaultObjectState::xiiDefaultObjectState(xiiObjectAccessorBase* pAccessor, const xiiArrayPtr<xiiPropertySelection> selection)
{
  m_pAccessor = pAccessor;
  m_Selection = selection;
  m_Providers.Reserve(m_Selection.GetCount());
  for (const xiiPropertySelection& sel : m_Selection)
  {
    auto& pProviders = m_Providers.ExpandAndGetRef();
    for (auto& func : xiiDefaultState::s_Factories)
    {
      xiiSharedPtr<xiiDefaultStateProvider> pProvider = func(pAccessor, sel.m_pObject, nullptr);
      if (pProvider != nullptr)
      {
        pProviders.PushBack(std::move(pProvider));
      }
      pProviders.Sort([](const xiiSharedPtr<xiiDefaultStateProvider>& pA, const xiiSharedPtr<xiiDefaultStateProvider>& pB) -> bool { return pA->GetRootDepth() > pB->GetRootDepth(); });
    }
  }
}

xiiColorGammaUB xiiDefaultObjectState::GetBackgroundColor() const
{
  return m_Providers[0][0]->GetBackgroundColor();
}

xiiString xiiDefaultObjectState::GetStateProviderName() const
{
  return m_Providers[0][0]->GetStateProviderName();
}

bool xiiDefaultObjectState::IsDefaultValue(xiiStringView sProperty) const
{
  const xiiAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(sProperty);
  return IsDefaultValue(pProp);
}

bool xiiDefaultObjectState::IsDefaultValue(const xiiAbstractProperty* pProp) const
{
  const xiiUInt32 uiObjects = m_Providers.GetCount();
  for (xiiUInt32 i = 0; i < uiObjects; i++)
  {
    xiiDefaultStateProvider::SuperArray super       = m_Providers[i].GetArrayPtr().GetSubArray(1);
    const bool                          bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
    if (!bNewDefault)
      return false;
  }
  return true;
}

xiiStatus xiiDefaultObjectState::RevertProperty(xiiStringView sProperty)
{
  const xiiAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(sProperty);
  return RevertProperty(pProp);
}

xiiStatus xiiDefaultObjectState::RevertProperty(const xiiAbstractProperty* pProp)
{
  const xiiUInt32 uiObjects = m_Providers.GetCount();
  for (xiiUInt32 i = 0; i < uiObjects; i++)
  {
    xiiDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    xiiStatus                           res   = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
    if (res.Failed())
      return res;
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiDefaultObjectState::RevertObject()
{
  const xiiUInt32 uiObjects = m_Providers.GetCount();
  for (xiiUInt32 i = 0; i < uiObjects; i++)
  {
    xiiDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);

    xiiHybridArray<xiiAbstractProperty*, 32> properties;
    m_Selection[i].m_pObject->GetType()->GetAllProperties(properties);
    for (xiiAbstractProperty* pProp : properties)
    {
      if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Hidden | xiiPropertyFlags::ReadOnly))
        continue;
      xiiStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, pProp);
      if (res.Failed())
        return res;
    }
  }
  return xiiStatus(XII_SUCCESS);
}

xiiVariant xiiDefaultObjectState::GetDefaultValue(xiiStringView sProperty, xiiUInt32 uiSelectionIndex) const
{
  const xiiAbstractProperty* pProp = m_Selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(sProperty);
  return GetDefaultValue(pProp, uiSelectionIndex);
}

xiiVariant xiiDefaultObjectState::GetDefaultValue(const xiiAbstractProperty* pProp, xiiUInt32 uiSelectionIndex) const
{
  XII_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  xiiDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, pProp);
}

//////////////////////////////////////////////////////////////////////////

xiiDefaultContainerState::xiiDefaultContainerState(xiiObjectAccessorBase* pAccessor, const xiiArrayPtr<xiiPropertySelection> selection, xiiStringView sProperty)
{
  m_pAccessor = pAccessor;
  m_Selection = selection;
  // We assume selections can only contain objects of the same (base) type.
  m_pProp = !sProperty.IsEmpty() ? selection[0].m_pObject->GetTypeAccessor().GetType()->FindPropertyByName(sProperty) : nullptr;
  m_Providers.Reserve(m_Selection.GetCount());
  for (const xiiPropertySelection& sel : m_Selection)
  {
    auto& pProviders = m_Providers.ExpandAndGetRef();
    for (auto& func : xiiDefaultState::s_Factories)
    {
      xiiSharedPtr<xiiDefaultStateProvider> pProvider = func(pAccessor, sel.m_pObject, m_pProp);
      if (pProvider != nullptr)
      {
        pProviders.PushBack(std::move(pProvider));
      }
      pProviders.Sort([](const xiiSharedPtr<xiiDefaultStateProvider>& pA, const xiiSharedPtr<xiiDefaultStateProvider>& pB) -> bool { return pA->GetRootDepth() > pB->GetRootDepth(); });
    }
  }
}

xiiColorGammaUB xiiDefaultContainerState::GetBackgroundColor() const
{
  return m_Providers[0][0]->GetBackgroundColor();
}

xiiString xiiDefaultContainerState::GetStateProviderName() const
{
  return m_Providers[0][0]->GetStateProviderName();
}

bool xiiDefaultContainerState::IsDefaultElement(xiiVariant index) const
{
  const xiiUInt32 uiObjects = m_Providers.GetCount();
  for (xiiUInt32 i = 0; i < uiObjects; i++)
  {
    xiiDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    XII_ASSERT_DEBUG(index.IsValid() || m_Selection[i].m_Index.IsValid(), "If xiiDefaultContainerState is constructed without giving an indices in the selection, one must be provided on the IsDefaultElement call.");
    const bool bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp, index.IsValid() ? index : m_Selection[i].m_Index);
    if (!bNewDefault)
      return false;
  }
  return true;
}

bool xiiDefaultContainerState::IsDefaultContainer() const
{
  const xiiUInt32 uiObjects = m_Providers.GetCount();
  for (xiiUInt32 i = 0; i < uiObjects; i++)
  {
    xiiDefaultStateProvider::SuperArray super       = m_Providers[i].GetArrayPtr().GetSubArray(1);
    const bool                          bNewDefault = m_Providers[i][0]->IsDefaultValue(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp);
    if (!bNewDefault)
      return false;
  }
  return true;
}

xiiStatus xiiDefaultContainerState::RevertElement(xiiVariant index)
{
  const xiiUInt32 uiObjects = m_Providers.GetCount();
  for (xiiUInt32 i = 0; i < uiObjects; i++)
  {
    xiiDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    XII_ASSERT_DEBUG(index.IsValid() || m_Selection[i].m_Index.IsValid(), "If xiiDefaultContainerState is constructed without giving an indices in the selection, one must be provided on the RevertElement call.");
    xiiStatus res = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp, index.IsValid() ? index : m_Selection[i].m_Index);
    if (res.Failed())
      return res;
  }
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiDefaultContainerState::RevertContainer()
{
  const xiiUInt32 uiObjects = m_Providers.GetCount();
  for (xiiUInt32 i = 0; i < uiObjects; i++)
  {
    xiiDefaultStateProvider::SuperArray super = m_Providers[i].GetArrayPtr().GetSubArray(1);
    xiiStatus                           res   = m_Providers[i][0]->RevertProperty(super, m_pAccessor, m_Selection[i].m_pObject, m_pProp);
    if (res.Failed())
      return res;
  }
  return xiiStatus(XII_SUCCESS);
}

xiiVariant xiiDefaultContainerState::GetDefaultElement(xiiVariant index, xiiUInt32 uiSelectionIndex) const
{
  XII_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  xiiDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, m_pProp, index);
}

xiiVariant xiiDefaultContainerState::GetDefaultContainer(xiiUInt32 uiSelectionIndex) const
{
  XII_ASSERT_DEBUG(uiSelectionIndex < m_Selection.GetCount(), "Selection index is out of bounds.");
  xiiDefaultStateProvider::SuperArray super = m_Providers[uiSelectionIndex].GetArrayPtr().GetSubArray(1);
  return m_Providers[uiSelectionIndex][0]->GetDefaultValue(super, m_pAccessor, m_Selection[uiSelectionIndex].m_pObject, m_pProp);
}

//////////////////////////////////////////////////////////////////////////


bool xiiDefaultStateProvider::IsDefaultValue(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  const xiiVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
  xiiVariant       value;
  pAccessor->GetValue(pObject, pProp, value, index).LogFailure();

  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags);
  if (index.IsValid() && !bIsValueType)
  {
    //#TODO we do not support reverting entire objects just yet.
    return true;
  }

  return def == value;
}

xiiStatus xiiDefaultStateProvider::RevertProperty(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp, xiiVariant index)
{
  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags);
  if (!bIsValueType)
  {
    XII_ASSERT_DEBUG(!index.IsValid(), "Reverting non-value type container elements is not supported yet. IsDefaultValue should have returned true to prevent this call from being allowed.");

    return RevertObjectContainer(superPtr, pAccessor, pObject, pProp);
  }

  xiiDeque<xiiAbstractGraphDiffOperation> diff;
  auto&                                   op = diff.ExpandAndGetRef();
  op.m_Node                                  = pObject->GetGuid();
  op.m_Operation                             = xiiAbstractGraphDiffOperation::Op::PropertyChanged;
  op.m_sProperty                             = pProp->GetPropertyName();
  op.m_uiTypeVersion                         = 0;
  if (index.IsValid())
  {
    xiiVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        XII_ASSERT_DEBUG(index.CanConvertTo<xiiInt32>(), "Array / Set indices must be integers.");
        XII_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, op.m_Value));
        XII_ASSERT_DEBUG(op.m_Value.IsA<xiiVariantArray>(), "");

        xiiVariantArray& currentValue2              = op.m_Value.GetWritable<xiiVariantArray>();
        currentValue2[index.ConvertTo<xiiUInt32>()] = def;
      }
      break;
      case xiiPropertyCategory::Map:
      {
        XII_ASSERT_DEBUG(index.IsString(), "Map indices must be strings.");
        XII_SUCCEED_OR_RETURN(pAccessor->GetValue(pObject, pProp, op.m_Value));
        XII_ASSERT_DEBUG(op.m_Value.IsA<xiiVariantDictionary>(), "");

        xiiVariantDictionary& currentValue2         = op.m_Value.GetWritable<xiiVariantDictionary>();
        currentValue2[index.ConvertTo<xiiString>()] = def;
      }
      break;
      default:
        break;
    }
  }
  else
  {
    xiiVariant def = GetDefaultValue(superPtr, pAccessor, pObject, pProp, index);
    op.m_Value     = def;
  }

  xiiDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
  return xiiStatus(XII_SUCCESS);
}

xiiStatus xiiDefaultStateProvider::RevertObjectContainer(SuperArray superPtr, xiiObjectAccessorBase* pAccessor, const xiiDocumentObject* pObject, const xiiAbstractProperty* pProp)
{
  xiiDeque<xiiAbstractGraphDiffOperation> diff;
  xiiStatus                               res = CreateRevertContainerDiff(superPtr, pAccessor, pObject, pProp, diff);
  if (res.Succeeded())
  {
    xiiDocumentObjectConverterReader::ApplyDiffToObject(pAccessor, pObject, diff);
  }
  return res;
}

bool xiiDefaultStateProvider::DoesVariantMatchProperty(const xiiVariant& value, const xiiAbstractProperty* pProp, xiiVariant index)
{
  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp) || pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags);

  if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
    return true;

  auto MatchesElementType = [&](const xiiVariant& value2) -> bool {
    if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
    {
      return value2.IsNumber() && !value2.IsFloatingPoint();
    }
    else if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::StandardType))
    {
      return value2.CanConvertTo(pProp->GetSpecificType()->GetVariantType());
    }
    else if (bIsValueType)
    {
      return value2.GetReflectedType() == pProp->GetSpecificType();
    }
    else
    {
      return value2.IsA<xiiUuid>();
    }
  };

  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      return MatchesElementType(value);
    }
    break;
    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Set:
    {
      if (index.IsValid())
      {
        return MatchesElementType(value);
      }
      else
      {
        if (value.IsA<xiiVariantArray>())
        {
          const xiiVariantArray& valueArray = value.Get<xiiVariantArray>();
          return std::all_of(cbegin(valueArray), cend(valueArray), MatchesElementType);
        }
      }
    }
    break;
    case xiiPropertyCategory::Map:
    {
      if (index.IsValid())
      {
        return MatchesElementType(value);
      }
      else
      {
        if (value.IsA<xiiVariantDictionary>())
        {
          const xiiVariantDictionary& valueDict = value.Get<xiiVariantDictionary>();
          return std::all_of(cbegin(valueDict), cend(valueDict), [&](const auto& it) { return MatchesElementType(it.Value()); });
        }
      }
    }
    break;
    default:
      break;
  }
  return false;
}
