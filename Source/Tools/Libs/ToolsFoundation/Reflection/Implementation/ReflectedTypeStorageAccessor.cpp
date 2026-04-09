#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Reflection/VariantStorageAccessor.h>

////////////////////////////////////////////////////////////////////////
// xiiReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

xiiReflectedTypeStorageAccessor::xiiReflectedTypeStorageAccessor(const xiiRTTI* pRtti, xiiDocumentObject* pOwner) :
  xiiIReflectedTypeAccessor(pRtti, pOwner)
{
  const xiiRTTI* pType = pRtti;
  XII_ASSERT_DEV(pType != nullptr, "Trying to construct an xiiReflectedTypeStorageAccessor for an invalid type!");
  m_pMapping = xiiReflectedTypeStorageManager::AddStorageAccessor(this);
  XII_ASSERT_DEV(m_pMapping != nullptr, "The type for this xiiReflectedTypeStorageAccessor is unknown to the xiiReflectedTypeStorageManager!");

  auto&           indexTable   = m_pMapping->m_PathToStorageInfoTable;
  const xiiUInt32 uiProperties = indexTable.GetCount();
  // To prevent re-allocs due to new properties being added we reserve 20% more space.
  m_Data.Reserve(uiProperties + uiProperties / 20);
  m_Data.SetCount(uiProperties);

  // Fill data storage with default values for the given types.
  for (auto it = indexTable.GetIterator(); it.IsValid(); ++it)
  {
    const auto& storageInfo       = it.Value();
    m_Data[storageInfo.m_uiIndex] = storageInfo.m_DefaultValue;
  }
}

xiiReflectedTypeStorageAccessor::~xiiReflectedTypeStorageAccessor()
{
  xiiReflectedTypeStorageManager::RemoveStorageAccessor(this);
}

const xiiVariant xiiReflectedTypeStorageAccessor::GetValue(xiiStringView sProperty, xiiVariant index, xiiStatus* pRes) const
{
  const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
  if (pProp == nullptr)
  {
    if (pRes)
      *pRes = xiiStatus(xiiFmt("Property '{0}' not found in type '{1}'", sProperty, GetType()->GetTypeName()));
    return xiiVariant();
  }

  if (pRes)
    *pRes = xiiStatus(XII_SUCCESS);
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* pStorageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, pStorageInfo))
  {
    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Member:
        if (index.IsValid())
        {
          if (pRes)
          {
            *pRes = xiiStatus(xiiFmt("Property '{0}' is a member property but an index of '{1}' is given", sProperty, index));
          }
          return xiiVariant();
        }
        return m_Data[pStorageInfo->m_uiIndex];
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      case xiiPropertyCategory::Map:
      {
        return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).GetValue(index, pRes);
      }
      break;
      default:
        break;
    }
  }
  return xiiVariant();
}

bool xiiReflectedTypeStorageAccessor::SetValue(xiiStringView sProperty, const xiiVariant& value, xiiVariant index)
{
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* pStorageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, pStorageInfo))
  {
    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;
    XII_ASSERT_DEV(pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>() || value.IsValid(), "");

    if (pStorageInfo->m_Type == xiiVariantType::TypedObject && pStorageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool                 isValueType = xiiReflectionUtils::IsValueType(pProp);
    const xiiVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(xiiPropertyFlags::Class) && !isValueType) ? xiiVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Member:
      {
        if (index.IsValid())
          return false;

        if (value.IsA<xiiString>() && pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        {
          xiiInt64 iValue;
          xiiReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<xiiString>(), iValue);
          m_Data[pStorageInfo->m_uiIndex] = xiiVariant(iValue).ConvertTo(pStorageInfo->m_Type);
          return true;
        }
        else if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
        {
          m_Data[pStorageInfo->m_uiIndex] = value;
          return true;
        }
        else if (value.CanConvertTo(pStorageInfo->m_Type))
        {
          // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
          // that may have a different type now as someone reloaded the type information and replaced a type.
          m_Data[pStorageInfo->m_uiIndex] = value.ConvertTo(pStorageInfo->m_Type != xiiVariantType::StringView ? (xiiVariantType::Enum)pStorageInfo->m_Type : value.GetType());
          return true;
        }
      }
      break;
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        if (index.IsNumber())
        {
          if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
            return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).SetValue(value, index).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).SetValue(value.ConvertTo(SpecVarType != xiiVariantType::StringView ? SpecVarType : value.GetType()), index).Succeeded();
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        if (index.IsA<xiiString>() || index.IsA<xiiStringView>())
        {
          if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
            return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).SetValue(value, index.ConvertTo<xiiString>()).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).SetValue(value.ConvertTo(SpecVarType != xiiVariantType::StringView ? SpecVarType : value.GetType()), index).Succeeded();
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

xiiInt32 xiiReflectedTypeStorageAccessor::GetCount(xiiStringView sProperty) const
{
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* pStorageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, pStorageInfo))
  {
    if (pStorageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return -1;

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      case xiiPropertyCategory::Map:
        return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).GetCount();
      default:
        break;
    }
  }
  return -1;
}

bool xiiReflectedTypeStorageAccessor::GetKeys(xiiStringView sProperty, xiiDynamicArray<xiiVariant>& out_keys) const
{
  out_keys.Clear();

  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* pStorageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, pStorageInfo))
  {
    if (pStorageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      case xiiPropertyCategory::Map:
      {
        return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).GetKeys(out_keys).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}
bool xiiReflectedTypeStorageAccessor::InsertValue(xiiStringView sProperty, xiiVariant index, const xiiVariant& value)
{
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* pStorageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, pStorageInfo))
  {
    if (pStorageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    if (pStorageInfo->m_Type == xiiVariantType::TypedObject && pStorageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool                 isValueType = xiiReflectionUtils::IsValueType(pProp);
    const xiiVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(xiiPropertyFlags::Class) && !isValueType) ? xiiVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        if (index.IsNumber())
        {
          if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
            return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).InsertValue(index, value).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).InsertValue(index, value.ConvertTo(SpecVarType != xiiVariantType::StringView ? SpecVarType : value.GetType())).Succeeded();
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        if (index.IsA<xiiString>() || index.IsA<xiiStringView>())
        {
          if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
            return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).InsertValue(index.ConvertTo<xiiString>(), value).Succeeded();
          else if (value.CanConvertTo(SpecVarType))
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).InsertValue(index, value.ConvertTo(SpecVarType != xiiVariantType::StringView ? SpecVarType : value.GetType())).Succeeded();
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool xiiReflectedTypeStorageAccessor::RemoveValue(xiiStringView sProperty, xiiVariant index)
{
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* pStorageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, pStorageInfo))
  {
    if (pStorageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      case xiiPropertyCategory::Map:
      {
        return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).RemoveValue(index).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool xiiReflectedTypeStorageAccessor::MoveValue(xiiStringView sProperty, xiiVariant oldIndex, xiiVariant newIndex)
{
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* pStorageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, pStorageInfo))
  {
    if (pStorageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      case xiiPropertyCategory::Map:
      {
        return xiiVariantStorageAccessor(sProperty, m_Data[pStorageInfo->m_uiIndex]).MoveValue(oldIndex, newIndex).Succeeded();
      }
      break;
      default:
        break;
    }
  }
  return false;
}

xiiVariant xiiReflectedTypeStorageAccessor::GetPropertyChildIndex(xiiStringView sProperty, const xiiVariant& value) const
{
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* pStorageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, pStorageInfo))
  {
    // if (pStorageInfo->m_Type == xiiVariant::Type::Invalid)
    //   return xiiVariant();

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return xiiVariant();

    const bool                 isValueType = xiiReflectionUtils::IsValueType(pProp);
    const xiiVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(xiiPropertyFlags::Class) && !isValueType) ? xiiVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const xiiVariantArray& values = m_Data[pStorageInfo->m_uiIndex].Get<xiiVariantArray>();
          for (xiiUInt32 i = 0; i < values.GetCount(); i++)
          {
            if (values[i] == value)
              return xiiVariant((xiiUInt32)i);
          }
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const xiiVariantDictionary& values = m_Data[pStorageInfo->m_uiIndex].Get<xiiVariantDictionary>();
          for (auto it = values.GetIterator(); it.IsValid(); ++it)
          {
            if (it.Value() == value)
              return xiiVariant(it.Key());
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return xiiVariant();
}
