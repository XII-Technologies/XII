/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

namespace
{
  static const char* s_DataOffsetSourceNames[] = {
    "Local",
    "Instance",
    "Constant",
  };
  static_assert(XII_ARRAY_SIZE(s_DataOffsetSourceNames) == (size_t)xiiVisualScriptDataDescription::DataOffset::Source::Count);
} // namespace

// Check that DataOffset fits in one uint32 and also check that we have enough bits for dataType and source.
static_assert(sizeof(xiiVisualScriptDataDescription::DataOffset) == sizeof(xiiUInt32));
static_assert(xiiVisualScriptDataType::Count <= XII_BIT(xiiVisualScriptDataDescription::DataOffset::TYPE_BITS));
static_assert(xiiVisualScriptDataDescription::DataOffset::Source::Count <= XII_BIT(xiiVisualScriptDataDescription::DataOffset::SOURCE_BITS));

// static
const char* xiiVisualScriptDataDescription::DataOffset::Source::GetName(Enum source)
{
  XII_ASSERT_DEBUG(source >= 0 && static_cast<xiiUInt32>(source) < XII_ARRAY_SIZE(s_DataOffsetSourceNames), "Out of bounds access");
  return s_DataOffsetSourceNames[source];
}

//////////////////////////////////////////////////////////////////////////

static const xiiTypeVersion s_uiVisualScriptDataDescriptionVersion = 2;

xiiResult xiiVisualScriptDataDescription::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiVisualScriptDataDescriptionVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream << typeInfo.m_uiCount;
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptDataDescription::Deserialize(xiiStreamReader& inout_stream)
{
  xiiTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptDataDescriptionVersion);
  if (uiVersion < 2)
  {
    xiiLog::Error("Invalid visual script data desc version. Expected >= 2 but got {}. Visual Script needs re-export", uiVersion);
    return XII_FAILURE;
  }

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream >> typeInfo.m_uiCount;
  }

  CalculatePerTypeStartOffsets();

  return XII_SUCCESS;
}

void xiiVisualScriptDataDescription::Clear()
{
  xiiMemoryUtils::ZeroFillArray(m_PerTypeInfo);
  m_uiStorageSizeNeeded = 0;
}

void xiiVisualScriptDataDescription::CalculatePerTypeStartOffsets()
{
  xiiUInt32 uiOffset = 0;
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(m_PerTypeInfo); ++i)
  {
    auto  dataType = static_cast<xiiVisualScriptDataType::Enum>(i);
    auto& typeInfo = m_PerTypeInfo[i];

    if (typeInfo.m_uiCount > 0)
    {
      uiOffset                 = xiiMemoryUtils::AlignSize(uiOffset, xiiVisualScriptDataType::GetStorageAlignment(dataType));
      typeInfo.m_uiStartOffset = uiOffset;

      uiOffset += xiiVisualScriptDataType::GetStorageSize(dataType) * typeInfo.m_uiCount;
    }
  }

  m_uiStorageSizeNeeded = uiOffset;
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptDataStorage::xiiVisualScriptDataStorage(const xiiSharedPtr<const xiiVisualScriptDataDescription>& pDesc) :
  m_pDesc(pDesc)
{
}

xiiVisualScriptDataStorage::~xiiVisualScriptDataStorage()
{
  DeallocateStorage();
}

void xiiVisualScriptDataStorage::AllocateStorage(xiiAllocator* pAllocator)
{
  XII_ASSERT_DEV(IsAllocated() == false, "Storage already allocated");

  m_Storage = XII_NEW_ARRAY(pAllocator, xiiUInt8, m_pDesc->m_uiStorageSizeNeeded);
  xiiMemoryUtils::ZeroFill(m_Storage.GetPtr(), m_Storage.GetCount());
  m_pAllocator = pAllocator;

  auto pData = m_Storage.GetPtr();

  for (xiiUInt32 scriptDataType = 0; scriptDataType < xiiVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == xiiVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<xiiString*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct<SkipTrivialTypes>(pStrings, typeInfo.m_uiCount);
    }
    if (scriptDataType == xiiVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<xiiHashedString*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct<SkipTrivialTypes>(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<xiiVariant*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct<SkipTrivialTypes>(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<xiiVariantArray*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct<SkipTrivialTypes>(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<xiiVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct<SkipTrivialTypes>(pVariantMaps, typeInfo.m_uiCount);
    }
  }
}

void xiiVisualScriptDataStorage::DeallocateStorage()
{
  if (IsAllocated() == false)
    return;

  auto pData = m_Storage.GetPtr();

  for (xiiUInt32 scriptDataType = 0; scriptDataType < xiiVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == xiiVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<xiiString*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Destruct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<xiiHashedString*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Destruct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<xiiVariant*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Destruct(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<xiiVariantArray*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Destruct(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<xiiVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Destruct(pVariantMaps, typeInfo.m_uiCount);
    }
  }

  XII_DELETE_ARRAY(m_pAllocator, m_Storage);
  m_pAllocator = nullptr;
}

xiiResult xiiVisualScriptDataStorage::Serialize(xiiStreamWriter& inout_stream) const
{
  auto pData = m_Storage.GetPtr();

  for (xiiUInt32 scriptDataType = 0; scriptDataType < xiiVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == xiiVisualScriptDataType::String)
    {
      auto pStrings    = reinterpret_cast<const xiiString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream << *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::HashedString)
    {
      auto pStrings    = reinterpret_cast<const xiiHashedString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream << *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::Variant)
    {
      auto pVariants    = reinterpret_cast<const xiiVariant*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantsEnd = pVariants + typeInfo.m_uiCount;
      while (pVariants < pVariantsEnd)
      {
        inout_stream << *pVariants;
        ++pVariants;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::Array)
    {
      auto pVariantArrays    = reinterpret_cast<const xiiVariantArray*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantArraysEnd = pVariantArrays + typeInfo.m_uiCount;
      while (pVariantArrays < pVariantArraysEnd)
      {
        XII_SUCCEED_OR_RETURN(inout_stream.WriteArray(*pVariantArrays));
        ++pVariantArrays;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::Map)
    {
      auto pVariantMaps    = reinterpret_cast<const xiiVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantMapsEnd = pVariantMaps + typeInfo.m_uiCount;
      while (pVariantMaps < pVariantMapsEnd)
      {
        XII_SUCCEED_OR_RETURN(inout_stream.WriteHashTable(*pVariantMaps));
        ++pVariantMaps;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::GameObject || scriptDataType == xiiVisualScriptDataType::Component || scriptDataType == xiiVisualScriptDataType::TypedPointer || scriptDataType == xiiVisualScriptDataType::Coroutine)
    {
      xiiLog::Error("Cannot serialize visual script data type '{}'", xiiVisualScriptDataType::GetName(static_cast<xiiVisualScriptDataType::Enum>(scriptDataType)));
      return XII_FAILURE;
    }
    else
    {
      const xiiUInt32 uiBytesToWrite = typeInfo.m_uiCount * xiiVisualScriptDataType::GetStorageSize(static_cast<xiiVisualScriptDataType::Enum>(scriptDataType));
      XII_SUCCEED_OR_RETURN(inout_stream.WriteBytes(pData + typeInfo.m_uiStartOffset, uiBytesToWrite));
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptDataStorage::Deserialize(xiiStreamReader& inout_stream, xiiAllocator* pAllocator)
{
  if (IsAllocated() == false)
  {
    AllocateStorage(pAllocator);
  }

  auto pData = m_Storage.GetPtr();

  for (xiiUInt32 scriptDataType = 0; scriptDataType < xiiVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == xiiVisualScriptDataType::String)
    {
      auto pStrings    = reinterpret_cast<xiiString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream >> *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::HashedString)
    {
      auto pStrings    = reinterpret_cast<xiiHashedString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream >> *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::Variant)
    {
      auto pVariants    = reinterpret_cast<xiiVariant*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantsEnd = pVariants + typeInfo.m_uiCount;
      while (pVariants < pVariantsEnd)
      {
        inout_stream >> *pVariants;
        ++pVariants;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::Array)
    {
      auto pVariantArrays    = reinterpret_cast<xiiVariantArray*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantArraysEnd = pVariantArrays + typeInfo.m_uiCount;
      while (pVariantArrays < pVariantArraysEnd)
      {
        XII_SUCCEED_OR_RETURN(inout_stream.ReadArray(*pVariantArrays));
        ++pVariantArrays;
      }
    }
    else if (scriptDataType == xiiVisualScriptDataType::Map)
    {
      auto pVariantMaps    = reinterpret_cast<xiiVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantMapsEnd = pVariantMaps + typeInfo.m_uiCount;
      while (pVariantMaps < pVariantMapsEnd)
      {
        XII_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(*pVariantMaps));
        ++pVariantMaps;
      }
    }
    else
    {
      const xiiUInt32 uiBytesToRead = typeInfo.m_uiCount * xiiVisualScriptDataType::GetStorageSize(static_cast<xiiVisualScriptDataType::Enum>(scriptDataType));
      inout_stream.ReadBytes(pData + typeInfo.m_uiStartOffset, uiBytesToRead);
    }
  }

  return XII_SUCCESS;
}

xiiTypedPointer xiiVisualScriptDataStorage::GetPointerData(DataOffset dataOffset, xiiUInt32 uiExecutionCounter) const
{
  m_pDesc->CheckOffset(dataOffset, nullptr);
  auto pData = m_Storage.GetPtr() + dataOffset.m_uiByteOffset;

  if (dataOffset.m_uiType == xiiVisualScriptDataType::GameObject)
  {
    auto& gameObjectHandle = *reinterpret_cast<const xiiVisualScriptGameObjectHandle*>(pData);
    return xiiTypedPointer(gameObjectHandle.GetPtr(uiExecutionCounter), xiiGetStaticRTTI<xiiGameObject>());
  }
  else if (dataOffset.m_uiType == xiiVisualScriptDataType::Component)
  {
    auto&         componentHandle = *reinterpret_cast<const xiiVisualScriptComponentHandle*>(pData);
    xiiComponent* pComponent      = componentHandle.GetPtr(uiExecutionCounter);
    return xiiTypedPointer(pComponent, pComponent != nullptr ? pComponent->GetDynamicRTTI() : nullptr);
  }
  else if (dataOffset.m_uiType == xiiVisualScriptDataType::TypedPointer)
  {
    return *reinterpret_cast<const xiiTypedPointer*>(pData);
  }

  xiiTypedPointer t;
  t.m_pObject = const_cast<xiiUInt8*>(pData);
  t.m_pType   = xiiVisualScriptDataType::GetRtti(static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiType));
  return t;
}

xiiVariant xiiVisualScriptDataStorage::GetDataAsVariant(DataOffset dataOffset, const xiiRTTI* pExpectedType, xiiUInt32 uiExecutionCounter) const
{
  auto scriptDataType = dataOffset.GetType();

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // pExpectedType == nullptr means that the caller expects a xiiVariant so we decide solely based on the scriptDataType.
  // We set the pExpectedType to the equivalent of the scriptDataType here so we don't need to check for pExpectedType == nullptr in all the asserts below.
  if (pExpectedType == nullptr || pExpectedType == xiiGetStaticRTTI<xiiVariant>())
  {
    pExpectedType = xiiVisualScriptDataType::GetRtti(scriptDataType);
  }
#endif

  switch (scriptDataType)
  {
    case xiiVisualScriptDataType::Invalid:
      return xiiVariant();

    case xiiVisualScriptDataType::Bool:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<bool>(), "");
      return GetData<bool>(dataOffset);

    case xiiVisualScriptDataType::Byte:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiUInt8>(), "");
      return GetData<xiiUInt8>(dataOffset);

    case xiiVisualScriptDataType::Int:
      if (pExpectedType == xiiGetStaticRTTI<xiiInt16>())
      {
        return static_cast<xiiInt16>(GetData<xiiInt32>(dataOffset));
      }
      else if (pExpectedType == xiiGetStaticRTTI<xiiUInt16>())
      {
        return static_cast<xiiUInt16>(GetData<xiiInt32>(dataOffset));
      }
      else if (pExpectedType == xiiGetStaticRTTI<xiiInt32>())
      {
        return GetData<xiiInt32>(dataOffset);
      }
      else
      {
        return static_cast<xiiUInt32>(GetData<xiiInt32>(dataOffset));
      }
      XII_ASSERT_NOT_IMPLEMENTED;

    case xiiVisualScriptDataType::Int64:
      XII_ASSERT_DEBUG(pExpectedType->GetTypeFlags().IsSet(xiiTypeFlags::IsEnum) || pExpectedType->GetTypeFlags().IsSet(xiiTypeFlags::Bitflags) || pExpectedType == xiiGetStaticRTTI<xiiInt64>(), "");
      return GetData<xiiInt64>(dataOffset);

    case xiiVisualScriptDataType::Float:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<float>(), "");
      return GetData<float>(dataOffset);

    case xiiVisualScriptDataType::Double:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<double>(), "");
      return GetData<double>(dataOffset);

    case xiiVisualScriptDataType::Color:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiColor>(), "");
      return GetData<xiiColor>(dataOffset);

    case xiiVisualScriptDataType::Vector3:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiVec3>(), "");
      return GetData<xiiVec3>(dataOffset);

    case xiiVisualScriptDataType::Quaternion:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiQuat>(), "");
      return GetData<xiiQuat>(dataOffset);

    case xiiVisualScriptDataType::Transform:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiTransform>(), "");
      return GetData<xiiTransform>(dataOffset);

    case xiiVisualScriptDataType::Time:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiTime>(), "");
      return GetData<xiiTime>(dataOffset);

    case xiiVisualScriptDataType::Angle:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiAngle>(), "");
      return GetData<xiiAngle>(dataOffset);

    case xiiVisualScriptDataType::String:
      if (pExpectedType == nullptr || pExpectedType == xiiGetStaticRTTI<xiiString>() || pExpectedType == xiiGetStaticRTTI<const char*>())
      {
        return GetData<xiiString>(dataOffset);
      }
      else if (pExpectedType == xiiGetStaticRTTI<xiiStringView>())
      {
        return xiiVariant(GetData<xiiString>(dataOffset).GetView(), false);
      }
      XII_ASSERT_NOT_IMPLEMENTED;

    case xiiVisualScriptDataType::HashedString:
      if (pExpectedType == nullptr || pExpectedType == xiiGetStaticRTTI<xiiHashedString>())
      {
        return GetData<xiiHashedString>(dataOffset);
      }
      else if (pExpectedType == xiiGetStaticRTTI<xiiTempHashedString>())
      {
        return xiiTempHashedString(GetData<xiiHashedString>(dataOffset));
      }
      XII_ASSERT_NOT_IMPLEMENTED;

    case xiiVisualScriptDataType::GameObject:
      if (pExpectedType == nullptr || pExpectedType == xiiGetStaticRTTI<xiiGameObject>())
      {
        return GetPointerData(dataOffset, uiExecutionCounter);
      }
      else if (pExpectedType == xiiGetStaticRTTI<xiiGameObjectHandle>())
      {
        return GetData<xiiGameObjectHandle>(dataOffset);
      }
      XII_ASSERT_NOT_IMPLEMENTED;

    case xiiVisualScriptDataType::Component:
      if (pExpectedType == nullptr || pExpectedType->IsDerivedFrom<xiiComponent>())
      {
        return GetPointerData(dataOffset, uiExecutionCounter);
      }
      else if (pExpectedType == xiiGetStaticRTTI<xiiComponentHandle>())
      {
        return GetData<xiiComponentHandle>(dataOffset);
      }
      XII_ASSERT_NOT_IMPLEMENTED;

    case xiiVisualScriptDataType::TypedPointer:
      return GetPointerData(dataOffset, uiExecutionCounter);

    case xiiVisualScriptDataType::Variant:
      return GetData<xiiVariant>(dataOffset);

    case xiiVisualScriptDataType::Array:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiVariantArray>(), "");
      return GetData<xiiVariantArray>(dataOffset);

    case xiiVisualScriptDataType::Map:
      XII_ASSERT_DEBUG(pExpectedType == xiiGetStaticRTTI<xiiVariantDictionary>(), "");
      return GetData<xiiVariantDictionary>(dataOffset);

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiVariant();
}

void xiiVisualScriptDataStorage::SetDataFromVariant(DataOffset dataOffset, const xiiVariant& value, xiiUInt32 uiExecutionCounter)
{
  if (dataOffset.IsValid() == false)
    return;

  auto scriptDataType = dataOffset.GetType();
  switch (scriptDataType)
  {
    case xiiVisualScriptDataType::Bool:
      SetData(dataOffset, value.Get<bool>());
      break;
    case xiiVisualScriptDataType::Byte:
      if (value.IsA<xiiInt8>())
      {
        SetData(dataOffset, xiiUInt8(value.Get<xiiInt8>()));
      }
      else
      {
        SetData(dataOffset, value.Get<xiiUInt8>());
      }
      break;
    case xiiVisualScriptDataType::Int:
      if (value.IsA<xiiInt16>())
      {
        SetData(dataOffset, xiiInt32(value.Get<xiiInt16>()));
      }
      else if (value.IsA<xiiUInt16>())
      {
        SetData(dataOffset, xiiInt32(value.Get<xiiUInt16>()));
      }
      else if (value.IsA<xiiInt32>())
      {
        SetData(dataOffset, value.Get<xiiInt32>());
      }
      else
      {
        SetData(dataOffset, xiiInt32(value.Get<xiiUInt32>()));
      }
      break;
    case xiiVisualScriptDataType::Int64:
      if (value.IsA<xiiInt64>())
      {
        SetData(dataOffset, value.Get<xiiInt64>());
      }
      else
      {
        SetData(dataOffset, xiiInt64(value.Get<xiiUInt64>()));
      }
      break;
    case xiiVisualScriptDataType::Float:
      SetData(dataOffset, value.Get<float>());
      break;
    case xiiVisualScriptDataType::Double:
      SetData(dataOffset, value.Get<double>());
      break;
    case xiiVisualScriptDataType::Color:
      SetData(dataOffset, value.Get<xiiColor>());
      break;
    case xiiVisualScriptDataType::Vector3:
      SetData(dataOffset, value.Get<xiiVec3>());
      break;
    case xiiVisualScriptDataType::Quaternion:
      SetData(dataOffset, value.Get<xiiQuat>());
      break;
    case xiiVisualScriptDataType::Transform:
      SetData(dataOffset, value.Get<xiiTransform>());
      break;
    case xiiVisualScriptDataType::Time:
      SetData(dataOffset, value.Get<xiiTime>());
      break;
    case xiiVisualScriptDataType::Angle:
      SetData(dataOffset, value.Get<xiiAngle>());
      break;
    case xiiVisualScriptDataType::String:
      if (value.IsA<xiiStringView>())
      {
        SetData(dataOffset, xiiString(value.Get<xiiStringView>()));
      }
      else
      {
        SetData(dataOffset, value.Get<xiiString>());
      }
      break;
    case xiiVisualScriptDataType::HashedString:
      if (value.IsA<xiiTempHashedString>())
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        SetData(dataOffset, value.Get<xiiHashedString>());
      }
      break;
    case xiiVisualScriptDataType::GameObject:
      if (value.IsA<xiiGameObjectHandle>())
      {
        SetData(dataOffset, value.Get<xiiGameObjectHandle>());
      }
      else
      {
        SetPointerData(dataOffset, value.Get<xiiGameObject*>(), xiiGetStaticRTTI<xiiGameObject>(), uiExecutionCounter);
      }
      break;
    case xiiVisualScriptDataType::Component:
      if (value.IsA<xiiComponentHandle>())
      {
        SetData(dataOffset, value.Get<xiiComponentHandle>());
      }
      else
      {
        SetPointerData(dataOffset, value.Get<xiiComponent*>(), xiiGetStaticRTTI<xiiComponent>(), uiExecutionCounter);
      }
      break;
    case xiiVisualScriptDataType::TypedPointer:
    {
      xiiTypedPointer typedPtr = value.Get<xiiTypedPointer>();
      SetPointerData(dataOffset, typedPtr.m_pObject, typedPtr.m_pType, uiExecutionCounter);
    }
    break;
    case xiiVisualScriptDataType::Variant:
      SetData(dataOffset, value);
      break;
    case xiiVisualScriptDataType::Array:
      SetData(dataOffset, value.Get<xiiVariantArray>());
      break;
    case xiiVisualScriptDataType::Map:
      SetData(dataOffset, value.Get<xiiVariantDictionary>());
      break;
    case xiiVisualScriptDataType::Coroutine:
      SetData(dataOffset, value.Get<xiiScriptCoroutineHandle>());
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
