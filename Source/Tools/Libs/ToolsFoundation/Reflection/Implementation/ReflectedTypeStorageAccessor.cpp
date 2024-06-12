#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>


////////////////////////////////////////////////////////////////////////
// xiiReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

xiiReflectedTypeStorageAccessor::xiiReflectedTypeStorageAccessor(const xiiRTTI* pRtti, xiiDocumentObject* pOwner) :
  xiiIReflectedTypeAccessor(pRtti, pOwner)
{
  const xiiRTTI* pType = pRtti;
  XII_ASSERT_DEV(pType != nullptr, "Trying to construct a xiiReflectedTypeStorageAccessor for an invalid type!");
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
    const auto storageInfo        = it.Value();
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
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Member:
        return m_Data[storageInfo->m_uiIndex];
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        if (!index.IsValid())
        {
          return m_Data[storageInfo->m_uiIndex];
        }

        const xiiVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantArray>();
        if (index.CanConvertTo<xiiUInt32>())
        {
          xiiUInt32 uiIndex = index.ConvertTo<xiiUInt32>();
          if (uiIndex < values.GetCount())
          {
            return values[uiIndex];
          }
        }
        if (pRes)
          *pRes = xiiStatus(xiiFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, sProperty));
      }
      break;
      case xiiPropertyCategory::Map:
      {
        if (!index.IsValid())
        {
          return m_Data[storageInfo->m_uiIndex];
        }

        const xiiVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantDictionary>();
        if (index.IsA<xiiString>())
        {
          const xiiString& sIndex = index.Get<xiiString>();
          if (const xiiVariant* pValue = values.GetValue(sIndex))
          {
            return *pValue;
          }
        }
        if (pRes)
          *pRes = xiiStatus(xiiFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, sProperty));
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
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;
    XII_ASSERT_DEV(pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>() || value.IsValid(), "");

    if (storageInfo->m_Type == xiiVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
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
        if (value.IsA<xiiString>() && pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        {
          xiiInt64 iValue;
          xiiReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<xiiString>().GetView(), iValue);
          m_Data[storageInfo->m_uiIndex] = xiiVariant(iValue).ConvertTo(storageInfo->m_Type);
          return true;
        }
        else if (value.IsA<xiiStringView>() && pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags))
        {
          xiiInt64 iValue;
          xiiReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<xiiStringView>(), iValue);
          m_Data[storageInfo->m_uiIndex] = xiiVariant(iValue).ConvertTo(storageInfo->m_Type);
          return true;
        }
        else if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
        {
          m_Data[storageInfo->m_uiIndex] = value;
          return true;
        }
        else if (value.CanConvertTo(storageInfo->m_Type))
        {
          // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
          // that may have a different type now as someone reloaded the type information and replaced a type.
          m_Data[storageInfo->m_uiIndex] = value.ConvertTo(storageInfo->m_Type != xiiVariantType::StringView ? (xiiVariantType::Enum)storageInfo->m_Type : value.GetType());
          return true;
        }
      }
      break;
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        const xiiVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantArray>();
        if (index.CanConvertTo<xiiUInt32>())
        {
          xiiUInt32 uiIndex = index.ConvertTo<xiiUInt32>();
          if (uiIndex < values.GetCount())
          {
            xiiVariantArray changedValues = values;
            if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
            {
              changedValues[uiIndex]         = value;
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
            else
            {
              if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
              {
                m_Data[storageInfo->m_uiIndex] = value;
                return true;
              }
              else if (value.CanConvertTo(SpecVarType))
              {
                // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
                // that may have a different type now as someone reloaded the type information and replaced a type.
                changedValues[uiIndex]         = value.ConvertTo(SpecVarType != xiiVariantType::StringView ? SpecVarType : value.GetType());
                m_Data[storageInfo->m_uiIndex] = changedValues;
                return true;
              }
            }
          }
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        const xiiVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantDictionary>();
        if (index.IsA<xiiString>() && values.Contains(index.Get<xiiString>()))
        {
          const xiiString&     sIndex        = index.Get<xiiString>();
          xiiVariantDictionary changedValues = values;
          if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
          {
            changedValues[sIndex]          = value;
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
          else
          {
            if (value.CanConvertTo(SpecVarType))
            {
              // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
              // that may have a different type now as someone reloaded the type information and replaced a type.
              changedValues[sIndex]          = value.ConvertTo(SpecVarType != xiiVariantType::StringView ? SpecVarType : value.GetType());
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
          }
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
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == xiiVariant::Type::Invalid)
      return -1;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return -1;

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        const xiiVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantArray>();
        return values.GetCount();
      }
      case xiiPropertyCategory::Map:
      {
        const xiiVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantDictionary>();
        return values.GetCount();
      }
      default:
        break;
    }
  }
  return -1;
}

bool xiiReflectedTypeStorageAccessor::GetKeys(xiiStringView sProperty, xiiDynamicArray<xiiVariant>& out_keys) const
{
  out_keys.Clear();

  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        const xiiVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantArray>();
        out_keys.Reserve(values.GetCount());
        for (xiiUInt32 i = 0; i < values.GetCount(); ++i)
        {
          out_keys.PushBack(i);
        }
        return true;
      }
      break;
      case xiiPropertyCategory::Map:
      {
        const xiiVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantDictionary>();
        out_keys.Reserve(values.GetCount());
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          out_keys.PushBack(xiiVariant(it.Key()));
        }
        return true;
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
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    if (storageInfo->m_Type == xiiVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
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
        const xiiVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantArray>();
        if (index.CanConvertTo<xiiUInt32>())
        {
          xiiUInt32 uiIndex = index.ConvertTo<xiiUInt32>();
          if (uiIndex <= values.GetCount())
          {
            xiiVariantArray changedValues = values;
            if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
            {
              changedValues.InsertAt(uiIndex, value);
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
            else if (value.CanConvertTo(SpecVarType))
            {
              // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
              // that may have a different type now as someone reloaded the type information and replaced a type.
              changedValues.InsertAt(uiIndex, value.ConvertTo(SpecVarType));
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
          }
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        const xiiVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantDictionary>();
        if (index.IsA<xiiString>() && !values.Contains(index.Get<xiiString>()))
        {
          const xiiString&     sIndex        = index.Get<xiiString>();
          xiiVariantDictionary changedValues = values;
          if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
          {
            changedValues.Insert(sIndex, value);
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
          else if (value.CanConvertTo(SpecVarType))
          {
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            changedValues.Insert(sIndex, value.ConvertTo(SpecVarType));
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
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
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        const xiiVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantArray>();
        if (index.CanConvertTo<xiiUInt32>())
        {
          xiiUInt32 uiIndex = index.ConvertTo<xiiUInt32>();
          if (uiIndex < values.GetCount())
          {
            xiiVariantArray changedValues = values;
            changedValues.RemoveAtAndCopy(uiIndex);
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        const xiiVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantDictionary>();
        if (index.IsA<xiiString>() && values.Contains(index.Get<xiiString>()))
        {
          const xiiString&     sIndex        = index.Get<xiiString>();
          xiiVariantDictionary changedValues = values;
          changedValues.Remove(sIndex);
          m_Data[storageInfo->m_uiIndex] = changedValues;
          return true;
        }
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
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == xiiVariant::Type::Invalid)
      return false;

    const xiiAbstractProperty* pProp = GetType()->FindPropertyByName(sProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case xiiPropertyCategory::Array:
      case xiiPropertyCategory::Set:
      {
        const xiiVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantArray>();
        if (oldIndex.CanConvertTo<xiiUInt32>() && newIndex.CanConvertTo<xiiUInt32>())
        {
          xiiUInt32 uiOldIndex = oldIndex.ConvertTo<xiiUInt32>();
          xiiUInt32 uiNewIndex = newIndex.ConvertTo<xiiUInt32>();
          if (uiOldIndex < values.GetCount() && uiNewIndex <= values.GetCount())
          {
            xiiVariantArray changedValues = values;
            xiiVariant      value         = changedValues[uiOldIndex];
            changedValues.RemoveAtAndCopy(uiOldIndex);
            if (uiNewIndex > uiOldIndex)
            {
              uiNewIndex -= 1;
            }
            changedValues.InsertAt(uiNewIndex, value);

            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
      case xiiPropertyCategory::Map:
      {
        const xiiVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantDictionary>();
        if (oldIndex.IsA<xiiString>() && values.Contains(oldIndex.Get<xiiString>()) && newIndex.IsA<xiiString>())
        {
          const xiiString&     sIndex        = oldIndex.Get<xiiString>();
          xiiVariantDictionary changedValues = values;
          changedValues.Insert(newIndex.Get<xiiString>(), changedValues[sIndex]);
          changedValues.Remove(sIndex);
          m_Data[storageInfo->m_uiIndex] = changedValues;
          return true;
        }
      }
      default:
        break;
    }
  }
  return false;
}

xiiVariant xiiReflectedTypeStorageAccessor::GetPropertyChildIndex(xiiStringView sProperty, const xiiVariant& value) const
{
  const xiiReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(sProperty, storageInfo))
  {
    if (storageInfo->m_Type == xiiVariant::Type::Invalid)
      return xiiVariant();

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
          const xiiVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantArray>();
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
          const xiiVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<xiiVariantDictionary>();
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
