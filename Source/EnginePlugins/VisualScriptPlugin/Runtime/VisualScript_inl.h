#include "VisualScript.h"

// static
template <typename T, xiiUInt32 Size>
void xiiVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(xiiArrayPtr<const T> a, xiiUInt32& inout_uiAdditionalDataSize)
{
  if (a.GetCount() > Size)
  {
    inout_uiAdditionalDataSize = xiiMemoryUtils::AlignSize<xiiUInt32>(inout_uiAdditionalDataSize, XII_ALIGNMENT_OF(T));
    inout_uiAdditionalDataSize += a.GetCount() * sizeof(T);
  }
}

// static
template <typename T, xiiUInt32 Size>
void xiiVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(xiiUInt32 uiSize, xiiUInt32 uiAlignment, xiiUInt32& inout_uiAdditionalDataSize)
{
  if (uiSize > Size * sizeof(T))
  {
    inout_uiAdditionalDataSize = xiiMemoryUtils::AlignSize<xiiUInt32>(inout_uiAdditionalDataSize, uiAlignment);
    inout_uiAdditionalDataSize += uiSize;
  }
}

template <typename T, xiiUInt32 Size>
T* xiiVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::Init(xiiUInt8 uiCount, xiiUInt8*& inout_pAdditionalData)
{
  if (uiCount <= Size)
  {
    return m_Embedded;
  }

  inout_pAdditionalData = xiiMemoryUtils::AlignForwards(inout_pAdditionalData, XII_ALIGNMENT_OF(T));
  inout_pAdditionalData += uiCount * sizeof(T);

  m_Ptr = reinterpret_cast<T*>(inout_pAdditionalData);
  return m_Ptr;
}

template <typename T, xiiUInt32 Size>
xiiResult xiiVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::ReadFromStream(xiiUInt8& out_uiCount, xiiStreamReader& inout_stream, xiiUInt8*& inout_pAdditionalData)
{
  xiiUInt16 uiCount = 0;
  inout_stream >> uiCount;

  if (uiCount > xiiMath::MaxValue<xiiUInt8>())
  {
    return XII_FAILURE;
  }
  out_uiCount = static_cast<xiiUInt8>(uiCount);

  T*              pTargetPtr       = Init(out_uiCount, inout_pAdditionalData);
  const xiiUInt64 uiNumBytesToRead = uiCount * sizeof(T);
  if (inout_stream.ReadBytes(pTargetPtr, uiNumBytesToRead) != uiNumBytesToRead)
    return XII_FAILURE;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiUInt32 xiiVisualScriptGraphDescription::Node::GetExecutionIndex(xiiUInt32 uiSlot) const
{
  if (uiSlot < m_NumExecutionIndices)
  {
    return m_NumExecutionIndices <= XII_ARRAY_SIZE(m_ExecutionIndices.m_Embedded) ? m_ExecutionIndices.m_Embedded[uiSlot] : m_ExecutionIndices.m_Ptr[uiSlot];
  }

  return xiiInvalidIndex;
}

XII_ALWAYS_INLINE xiiVisualScriptGraphDescription::DataOffset xiiVisualScriptGraphDescription::Node::GetInputDataOffset(xiiUInt32 uiSlot) const
{
  if (uiSlot < m_NumInputDataOffsets)
  {
    return m_NumInputDataOffsets <= XII_ARRAY_SIZE(m_InputDataOffsets.m_Embedded) ? m_InputDataOffsets.m_Embedded[uiSlot] : m_InputDataOffsets.m_Ptr[uiSlot];
  }

  return {};
}

XII_ALWAYS_INLINE xiiVisualScriptGraphDescription::DataOffset xiiVisualScriptGraphDescription::Node::GetOutputDataOffset(xiiUInt32 uiSlot) const
{
  if (uiSlot < m_NumOutputDataOffsets)
  {
    return m_NumOutputDataOffsets <= XII_ARRAY_SIZE(m_OutputDataOffsets.m_Embedded) ? m_OutputDataOffsets.m_Embedded[uiSlot] : m_OutputDataOffsets.m_Ptr[uiSlot];
  }

  return {};
}

template <typename T>
XII_ALWAYS_INLINE const T& xiiVisualScriptGraphDescription::Node::GetUserData() const
{
  XII_ASSERT_DEBUG(m_UserDataByteSize == sizeof(T), "Invalid data");
  return *reinterpret_cast<const T*>(m_UserDataByteSize <= sizeof(m_UserData.m_Embedded) ? m_UserData.m_Embedded : m_UserData.m_Ptr);
}

template <typename T>
void xiiVisualScriptGraphDescription::Node::SetUserData(const T& data, xiiUInt8*& inout_pAdditionalData)
{
  m_UserDataByteSize = sizeof(T);
  auto pUserData     = m_UserData.Init(m_UserDataByteSize / sizeof(xiiUInt32), inout_pAdditionalData);
  XII_CHECK_ALIGNMENT(pUserData, XII_ALIGNMENT_OF(T));
  *reinterpret_cast<T*>(pUserData) = data;
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE const xiiVisualScriptGraphDescription::Node* xiiVisualScriptGraphDescription::GetNode(xiiUInt32 uiIndex)
{
  return uiIndex < m_Nodes.GetCount() ? &m_Nodes.GetPtr()[uiIndex] : nullptr;
}

//////////////////////////////////////////////////////////////////////////

XII_FORCE_INLINE void xiiVisualScriptDataDescription::CheckOffset(DataOffset dataOffset, const xiiRTTI* pType) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  auto            expectedDataType = static_cast<xiiVisualScriptDataType::Enum>(dataOffset.m_uiDataType);
  auto&           offsetAndCount   = m_PerTypeInfo[expectedDataType];
  const xiiUInt32 uiLastOffset     = offsetAndCount.m_uiStartOffset + (offsetAndCount.m_uiCount - 1) * xiiVisualScriptDataType::GetStorageSize(expectedDataType);
  XII_ASSERT_DEBUG(dataOffset.m_uiByteOffset >= offsetAndCount.m_uiStartOffset && dataOffset.m_uiByteOffset <= uiLastOffset, "Invalid data offset");

  if (pType != nullptr)
  {
    auto givenDataType = xiiVisualScriptDataType::FromRtti(pType);
    XII_ASSERT_DEBUG(expectedDataType == givenDataType, "Data type mismatch, expected '{}' but got '{}'({})", xiiVisualScriptDataType::GetName(expectedDataType), xiiVisualScriptDataType::GetName(givenDataType), pType->GetTypeName());
  }
#endif
}

XII_FORCE_INLINE xiiVisualScriptDataDescription::DataOffset xiiVisualScriptDataDescription::GetOffset(xiiVisualScriptDataType::Enum dataType, xiiUInt32 uiIndex, bool bIsConstant) const
{
  auto&     offsetAndCount = m_PerTypeInfo[dataType];
  xiiUInt32 uiByteOffset   = xiiInvalidIndex;
  if (uiIndex < offsetAndCount.m_uiCount)
  {
    uiByteOffset = offsetAndCount.m_uiStartOffset + uiIndex * xiiVisualScriptDataType::GetStorageSize(dataType);
  }

  return DataOffset(uiByteOffset, dataType, bIsConstant);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
const T& xiiVisualScriptDataStorage::GetData(DataOffset dataOffset) const
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, xiiTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, xiiGetStaticRTTI<T>());

  return *reinterpret_cast<const T*>(m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
T& xiiVisualScriptDataStorage::GetWritableData(DataOffset dataOffset)
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, xiiTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, xiiGetStaticRTTI<T>());

  return *reinterpret_cast<T*>(m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
void xiiVisualScriptDataStorage::SetData(DataOffset dataOffset, const T& value)
{
  static_assert(!std::is_pointer<T>::value, "Use SetPointerData instead");

  if (dataOffset.m_uiByteOffset < m_Storage.GetByteBlobPtr().GetCount())
  {
    m_pDesc->CheckOffset(dataOffset, xiiGetStaticRTTI<T>());

    auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

    if constexpr (std::is_same<T, xiiGameObjectHandle>::value)
    {
      auto& storedHandle = *reinterpret_cast<xiiVisualScriptGameObjectHandle*>(pData);
      storedHandle.AssignHandle(value);
    }
    else if constexpr (std::is_same<T, xiiComponentHandle>::value)
    {
      auto& storedHandle = *reinterpret_cast<xiiVisualScriptComponentHandle*>(pData);
      storedHandle.AssignHandle(value);
    }
    else if constexpr (std::is_same<T, xiiStringView>::value)
    {
      *reinterpret_cast<xiiString*>(pData) = value;
    }
    else
    {
      *reinterpret_cast<T*>(pData) = value;
    }
  }
}

template <typename T>
void xiiVisualScriptDataStorage::SetPointerData(DataOffset dataOffset, T ptr, const xiiRTTI* pType, xiiUInt32 uiExecutionCounter)
{
  static_assert(std::is_pointer<T>::value);

  if (dataOffset.m_uiByteOffset < m_Storage.GetByteBlobPtr().GetCount())
  {
    auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

    if constexpr (std::is_same<T, xiiGameObject*>::value)
    {
      m_pDesc->CheckOffset(dataOffset, xiiGetStaticRTTI<xiiGameObject>());

      auto& storedHandle = *reinterpret_cast<xiiVisualScriptGameObjectHandle*>(pData);
      storedHandle.AssignPtr(ptr, uiExecutionCounter);
    }
    else if constexpr (std::is_same<T, xiiComponent*>::value)
    {
      m_pDesc->CheckOffset(dataOffset, xiiGetStaticRTTI<xiiComponent>());

      auto& storedHandle = *reinterpret_cast<xiiVisualScriptComponentHandle*>(pData);
      storedHandle.AssignPtr(ptr, uiExecutionCounter);
    }
    else
    {
      XII_ASSERT_DEBUG(pType->IsDerivedFrom<xiiComponent>() == false, "Component type '{}' is stored as typed pointer, cast to xiiComponent first to ensure correct storage", pType->GetTypeName());

      m_pDesc->CheckOffset(dataOffset, pType);

      auto& typedPointer     = *reinterpret_cast<xiiTypedPointer*>(pData);
      typedPointer.m_pObject = ptr;
      typedPointer.m_pType   = pType;
    }
  }
}
