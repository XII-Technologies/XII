/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_FORCE_INLINE void xiiVisualScriptDataDescription::CheckOffset(DataOffset dataOffset, const xiiRTTI* pType) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  auto  givenDataType  = dataOffset.GetType();
  auto& offsetAndCount = m_PerTypeInfo[givenDataType];
  XII_ASSERT_DEBUG(offsetAndCount.m_uiCount > 0, "Invalid data offset");
  const xiiUInt32 uiLastOffset = offsetAndCount.m_uiStartOffset + (offsetAndCount.m_uiCount - 1) * xiiVisualScriptDataType::GetStorageSize(givenDataType);
  XII_ASSERT_DEBUG(dataOffset.m_uiByteOffset >= offsetAndCount.m_uiStartOffset && dataOffset.m_uiByteOffset <= uiLastOffset, "Invalid data offset");

  if (pType != nullptr)
  {
    auto expectedDataType = xiiVisualScriptDataType::FromRtti(pType);
    XII_ASSERT_DEBUG(expectedDataType == givenDataType, "Data type mismatch, expected '{}'({}) but got '{}'", xiiVisualScriptDataType::GetName(expectedDataType), pType->GetTypeName(), xiiVisualScriptDataType::GetName(givenDataType));
  }
#endif
}

XII_FORCE_INLINE xiiVisualScriptDataDescription::DataOffset xiiVisualScriptDataDescription::GetOffset(xiiVisualScriptDataType::Enum dataType, xiiUInt32 uiIndex, DataOffset::Source::Enum source) const
{
  auto&     offsetAndCount = m_PerTypeInfo[dataType];
  xiiUInt32 uiByteOffset   = xiiInvalidIndex;
  if (uiIndex < offsetAndCount.m_uiCount)
  {
    uiByteOffset = offsetAndCount.m_uiStartOffset + uiIndex * xiiVisualScriptDataType::GetStorageSize(dataType);
  }

  return DataOffset(uiByteOffset, dataType, source);
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE const xiiVisualScriptDataDescription& xiiVisualScriptDataStorage::GetDesc() const
{
  return *m_pDesc;
}

XII_ALWAYS_INLINE bool xiiVisualScriptDataStorage::IsAllocated() const
{
  return m_Storage.IsEmpty() == false;
}

template <typename T>
const T& xiiVisualScriptDataStorage::GetData(DataOffset dataOffset) const
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, xiiTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, xiiGetStaticRTTI<T>());

  return *reinterpret_cast<const T*>(m_Storage.GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
T& xiiVisualScriptDataStorage::GetWritableData(DataOffset dataOffset)
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, xiiTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, xiiGetStaticRTTI<T>());

  return *reinterpret_cast<T*>(m_Storage.GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
void xiiVisualScriptDataStorage::SetData(DataOffset dataOffset, const T& value)
{
  static_assert(!std::is_pointer<T>::value, "Use SetPointerData instead");

  if (dataOffset.m_uiByteOffset < m_Storage.GetCount())
  {
    m_pDesc->CheckOffset(dataOffset, xiiGetStaticRTTI<T>());

    auto pData = m_Storage.GetPtr() + dataOffset.m_uiByteOffset;

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

  if (dataOffset.m_uiByteOffset < m_Storage.GetCount())
  {
    auto pData = m_Storage.GetPtr() + dataOffset.m_uiByteOffset;

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
      const bool bIsAllowedType = !pType || (pType->IsDerivedFrom<xiiComponent>() == false && pType->IsDerivedFrom<xiiGameObject>() == false);
      XII_ASSERT_DEBUG(bIsAllowedType, "GameObject or Component type '{}' is stored as typed pointer, cast to xiiGameObject or xiiComponent first to ensure correct storage", pType->GetTypeName());

      m_pDesc->CheckOffset(dataOffset, pType);

      auto& typedPointer     = *reinterpret_cast<xiiTypedPointer*>(pData);
      typedPointer.m_pObject = ptr;
      typedPointer.m_pType   = pType;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

inline xiiResult xiiVisualScriptInstanceData::Serialize(xiiStreamWriter& inout_stream) const
{
  XII_SUCCEED_OR_RETURN(m_DataOffset.Serialize(inout_stream));

  if (m_DataOffset.GetType() != xiiVisualScriptDataType::GameObject && m_DataOffset.GetType() != xiiVisualScriptDataType::Component && m_DataOffset.GetType() != xiiVisualScriptDataType::TypedPointer)
  {
    inout_stream << m_DefaultValue;
  }

  return XII_SUCCESS;
}

inline xiiResult xiiVisualScriptInstanceData::Deserialize(xiiStreamReader& inout_stream)
{
  XII_SUCCEED_OR_RETURN(m_DataOffset.Deserialize(inout_stream));

  if (m_DataOffset.GetType() == xiiVisualScriptDataType::GameObject)
  {
    m_DefaultValue = xiiGameObjectHandle();
  }
  else if (m_DataOffset.GetType() == xiiVisualScriptDataType::Component)
  {
    m_DefaultValue = xiiComponentHandle();
  }
  else if (m_DataOffset.GetType() == xiiVisualScriptDataType::TypedPointer)
  {
    m_DefaultValue = xiiTypedPointer();
  }
  else
  {
    inout_stream >> m_DefaultValue;
  }

  return XII_SUCCESS;
}
