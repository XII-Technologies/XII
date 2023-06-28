#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScript.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeFunctions.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

#include <Foundation/IO/StringDeduplicationContext.h>

namespace
{
  static const char* s_NodeDescTypeNames[] = {
    "", // Invalid,
    "EntryCall",
    "MessageHandler",
    "ReflectedFunction",
    "GetScriptOwner",

    "", // FirstBuiltin,

    "Builtin_Branch",
    "Builtin_And",
    "Builtin_Or",
    "Builtin_Not",
    "Builtin_Compare",
    "Builtin_IsValid",

    "Builtin_Add",
    "Builtin_Subtract",
    "Builtin_Multiply",
    "Builtin_Divide",

    "Builtin_ToBool",
    "Builtin_ToByte",
    "Builtin_ToInt",
    "Builtin_ToInt64",
    "Builtin_ToFloat",
    "Builtin_ToDouble",
    "Builtin_ToString",
    "Builtin_ToVariant",
    "Builtin_Variant_ConvertTo",

    "Builtin_MakeArray",

    "Builtin_TryGetComponentOfBaseType",

    "", // LastBuiltin,
  };
  static_assert(XII_ARRAY_SIZE(s_NodeDescTypeNames) == (size_t)xiiVisualScriptNodeDescription::Type::Count);

  template <typename T>
  xiiResult WriteNodeArray(xiiArrayPtr<T> a, xiiStreamWriter& inout_stream)
  {
    xiiUInt16 uiCount = static_cast<xiiUInt16>(a.GetCount());
    inout_stream << uiCount;

    return inout_stream.WriteBytes(a.GetPtr(), a.GetCount() * sizeof(T));
  }

} // namespace

// static
xiiVisualScriptNodeDescription::Type::Enum xiiVisualScriptNodeDescription::Type::GetConversionType(xiiVisualScriptDataType::Enum targetDataType)
{
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Bool - xiiVisualScriptDataType::Bool) == Builtin_ToBool);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Byte - xiiVisualScriptDataType::Bool) == Builtin_ToByte);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Int - xiiVisualScriptDataType::Bool) == Builtin_ToInt);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Int64 - xiiVisualScriptDataType::Bool) == Builtin_ToInt64);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Float - xiiVisualScriptDataType::Bool) == Builtin_ToFloat);
  static_assert(Builtin_ToBool + (xiiVisualScriptDataType::Double - xiiVisualScriptDataType::Bool) == Builtin_ToDouble);

  if (xiiVisualScriptDataType::IsNumber(targetDataType))
    return static_cast<Enum>(Builtin_ToBool + (targetDataType - xiiVisualScriptDataType::Bool));

  if (targetDataType == xiiVisualScriptDataType::String)
    return Builtin_ToString;

  if (targetDataType == xiiVisualScriptDataType::Variant)
    return Builtin_ToVariant;

  XII_ASSERT_NOT_IMPLEMENTED;
  return Invalid;
}

// static
const char* xiiVisualScriptNodeDescription::Type::GetName(Enum type)
{
  XII_ASSERT_DEBUG(type >= 0 && type < XII_ARRAY_SIZE(s_NodeDescTypeNames), "Out of bounds access");
  return s_NodeDescTypeNames[type];
}

void xiiVisualScriptNodeDescription::AppendUserDataName(xiiStringBuilder& out_sResult) const
{
  if (auto func = GetUserDataContext(m_Type).m_ToStringFunc)
  {
    out_sResult.Append(" ");

    func(*this, out_sResult);
  }
}

//////////////////////////////////////////////////////////////////////////

xiiVisualScriptGraphDescription::xiiVisualScriptGraphDescription()
{
  static_assert(sizeof(Node) == 64);
}

xiiVisualScriptGraphDescription::~xiiVisualScriptGraphDescription() = default;

static const xiiTypeVersion s_uiVisualScriptGraphDescriptionVersion = 1;

// static
xiiResult xiiVisualScriptGraphDescription::Serialize(xiiArrayPtr<const xiiVisualScriptNodeDescription> nodes, xiiStreamWriter& inout_stream)
{
  inout_stream.WriteVersion(s_uiVisualScriptGraphDescriptionVersion);

  xiiDefaultMemoryStreamStorage streamStorage;
  xiiMemoryStreamWriter         stream(&streamStorage);
  xiiUInt32                     additionalDataSize = 0;
  {
    for (auto& nodeDesc : nodes)
    {
      stream << nodeDesc.m_Type;
      stream << nodeDesc.m_DeductedDataType;
      XII_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_ExecutionIndices.GetArrayPtr(), stream));
      XII_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_InputDataOffsets.GetArrayPtr(), stream));
      XII_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_OutputDataOffsets.GetArrayPtr(), stream));

      ExecutionIndicesArray::AddAdditionalDataSize(nodeDesc.m_ExecutionIndices, additionalDataSize);
      InputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_InputDataOffsets, additionalDataSize);
      OutputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_OutputDataOffsets, additionalDataSize);

      if (auto func = GetUserDataContext(nodeDesc.m_Type).m_SerializeFunc)
      {
        xiiUInt32 uiSize      = 0;
        xiiUInt32 uiAlignment = 0;
        XII_SUCCEED_OR_RETURN(func(nodeDesc, stream, uiSize, uiAlignment));

        UserDataArray::AddAdditionalDataSize(uiSize, uiAlignment, additionalDataSize);
      }
    }
  }

  const xiiUInt32 uiRequiredStorageSize = nodes.GetCount() * sizeof(Node) + additionalDataSize;
  inout_stream << uiRequiredStorageSize;
  inout_stream << nodes.GetCount();

  return streamStorage.CopyToStream(inout_stream);
}

xiiResult xiiVisualScriptGraphDescription::Deserialize(xiiStreamReader& inout_stream)
{
  xiiTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptGraphDescriptionVersion);

  {
    xiiUInt32 uiStorageSize;
    inout_stream >> uiStorageSize;

    m_Storage.SetCountUninitialized(uiStorageSize);
    m_Storage.ZeroFill();
  }

  xiiUInt32 uiNumNodes;
  inout_stream >> uiNumNodes;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();
  auto nodes = xiiMakeArrayPtr(reinterpret_cast<Node*>(pData), uiNumNodes);

  xiiUInt8* pAdditionalData = pData + uiNumNodes * sizeof(Node);

  for (auto& node : nodes)
  {
    inout_stream >> node.m_Type;
    inout_stream >> node.m_DeductedDataType;

    node.m_Function = GetExecuteFunction(node.m_Type, node.m_DeductedDataType);

    XII_SUCCEED_OR_RETURN(node.m_ExecutionIndices.ReadFromStream(node.m_NumExecutionIndices, inout_stream, pAdditionalData));
    XII_SUCCEED_OR_RETURN(node.m_InputDataOffsets.ReadFromStream(node.m_NumInputDataOffsets, inout_stream, pAdditionalData));
    XII_SUCCEED_OR_RETURN(node.m_OutputDataOffsets.ReadFromStream(node.m_NumOutputDataOffsets, inout_stream, pAdditionalData));

    if (auto func = GetUserDataContext(node.m_Type).m_DeserializeFunc)
    {
      XII_SUCCEED_OR_RETURN(func(node, inout_stream, pAdditionalData));
    }
  }

  m_Nodes = nodes;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

static const xiiTypeVersion s_uiVisualScriptDataDescriptionVersion = 1;

xiiResult xiiVisualScriptDataDescription::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiVisualScriptDataDescriptionVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream << typeInfo.m_uiStartOffset;
    inout_stream << typeInfo.m_uiCount;
  }

  inout_stream << m_uiStorageSizeNeeded;

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptDataDescription::Deserialize(xiiStreamReader& inout_stream)
{
  xiiTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptDataDescriptionVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream >> typeInfo.m_uiStartOffset;
    inout_stream >> typeInfo.m_uiCount;
  }

  inout_stream >> m_uiStorageSizeNeeded;

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

void xiiVisualScriptDataStorage::AllocateStorage()
{
  m_Storage.SetCountUninitialized(m_pDesc->m_uiStorageSizeNeeded);
  m_Storage.ZeroFill();

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (xiiUInt32 scriptDataType = 0; scriptDataType < xiiVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == xiiVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<xiiString*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<xiiVariant*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<xiiVariantArray*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == xiiVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<xiiVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      xiiMemoryUtils::Construct(pVariantMaps, typeInfo.m_uiCount);
    }
  }
}

void xiiVisualScriptDataStorage::DeallocateStorage()
{
  if (m_Storage.GetByteBlobPtr().IsEmpty())
    return;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

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

  m_Storage.Clear();
}

xiiResult xiiVisualScriptDataStorage::Serialize(xiiStreamWriter& inout_stream) const
{
  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

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
    else
    {
      const xiiUInt32 uiBytesToWrite = typeInfo.m_uiCount * xiiVisualScriptDataType::GetStorageSize(static_cast<xiiVisualScriptDataType::Enum>(scriptDataType));
      XII_SUCCEED_OR_RETURN(inout_stream.WriteBytes(pData + typeInfo.m_uiStartOffset, uiBytesToWrite));
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiVisualScriptDataStorage::Deserialize(xiiStreamReader& inout_stream)
{
  if (m_Storage.GetByteBlobPtr().IsEmpty())
  {
    AllocateStorage();
  }

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

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

xiiTypedPointer xiiVisualScriptDataStorage::GetPointerData(DataOffset dataOffset, xiiUInt32 uiExecutionCounter)
{
  m_pDesc->CheckOffset(dataOffset, nullptr);
  auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

  if (dataOffset.m_uiDataType == xiiVisualScriptDataType::GameObject)
  {
    auto& gameObjectHandle = *reinterpret_cast<const xiiVisualScriptGameObjectHandle*>(pData);
    return xiiTypedPointer(gameObjectHandle.GetPtr(uiExecutionCounter), xiiGetStaticRTTI<xiiGameObject>());
  }
  else if (dataOffset.m_uiDataType == xiiVisualScriptDataType::Component)
  {
    auto&         componentHandle = *reinterpret_cast<const xiiVisualScriptComponentHandle*>(pData);
    xiiComponent* pComponent      = componentHandle.GetPtr(uiExecutionCounter);
    return xiiTypedPointer(pComponent, pComponent != nullptr ? pComponent->GetDynamicRTTI() : nullptr);
  }
  else if (dataOffset.m_uiDataType == xiiVisualScriptDataType::TypedPointer)
  {
    return *reinterpret_cast<const xiiTypedPointer*>(pData);
  }

  XII_ASSERT_NOT_IMPLEMENTED;
  return xiiTypedPointer();
}

xiiVariant xiiVisualScriptDataStorage::GetDataAsVariant(DataOffset dataOffset, xiiVariantType::Enum expectedType, xiiUInt32 uiExecutionCounter) const
{
  auto scriptDataType = static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiDataType);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // expectedType == Invalid means that the caller expects an xiiVariant so we decide solely based on the scriptDataType.
  // We set the expectedType to the equivalent of the scriptDataType here so we don't need to check for expectedType == Invalid in all the asserts below.
  if (expectedType == xiiVariantType::Invalid)
  {
    expectedType = xiiVisualScriptDataType::GetVariantType(scriptDataType);
  }
#endif

  switch (scriptDataType)
  {
    case xiiVisualScriptDataType::Bool:
      XII_ASSERT_DEBUG(expectedType == xiiVariantType::Bool, "");
      return GetData<bool>(dataOffset);
    case xiiVisualScriptDataType::Byte:
      XII_ASSERT_DEBUG(expectedType == xiiVariantType::UInt8, "");
      return GetData<xiiUInt8>(dataOffset);
    case xiiVisualScriptDataType::Int:
      XII_ASSERT_DEBUG(expectedType == xiiVariantType::Int32, "");
      return GetData<xiiInt32>(dataOffset);
    case xiiVisualScriptDataType::Int64:
      XII_ASSERT_DEBUG(expectedType == xiiVariantType::Int64, "");
      return GetData<xiiInt64>(dataOffset);
    case xiiVisualScriptDataType::Float:
      XII_ASSERT_DEBUG(expectedType == xiiVariantType::Float, "");
      return GetData<float>(dataOffset);
    case xiiVisualScriptDataType::Double:
      XII_ASSERT_DEBUG(expectedType == xiiVariantType::Double, "");
      return GetData<double>(dataOffset);
    case xiiVisualScriptDataType::String:
      if (expectedType == xiiVariantType::Invalid || expectedType == xiiVariantType::String)
      {
        return GetData<xiiString>(dataOffset);
      }
      else if (expectedType == xiiVariantType::StringView)
      {
        return GetData<xiiString>(dataOffset).GetView();
      }
      XII_ASSERT_NOT_IMPLEMENTED;
    case xiiVisualScriptDataType::Variant:
      return GetData<xiiVariant>(dataOffset);
    case xiiVisualScriptDataType::Array:
      XII_ASSERT_DEBUG(expectedType == xiiVariantType::VariantArray, "");
      return GetData<xiiVariantArray>(dataOffset);
    case xiiVisualScriptDataType::Map:
      XII_ASSERT_DEBUG(expectedType == xiiVariantType::VariantDictionary, "");
      return GetData<xiiVariantDictionary>(dataOffset);
      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return xiiVariant();
}

void xiiVisualScriptDataStorage::SetDataFromVariant(DataOffset dataOffset, const xiiVariant& value, xiiUInt32 uiExecutionCounter)
{
  if (dataOffset.IsValid() == false)
    return;

  auto scriptDataType = static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiDataType);
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
    case xiiVisualScriptDataType::GameObject:
      SetPointerData(dataOffset, value.Get<xiiGameObject*>(), xiiGetStaticRTTI<xiiGameObject>(), uiExecutionCounter);
      break;
    case xiiVisualScriptDataType::Component:
      SetPointerData(dataOffset, value.Get<xiiComponent*>(), xiiGetStaticRTTI<xiiComponent>(), uiExecutionCounter);
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
      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
