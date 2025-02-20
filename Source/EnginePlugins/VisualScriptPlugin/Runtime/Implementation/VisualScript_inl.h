
// static
template <typename T, xiiUInt32 Size>
void xiiVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(xiiArrayPtr<const T> a, xiiUInt32& inout_uiAdditionalDataSize)
{
  if (a.GetCount() > Size)
  {
    inout_uiAdditionalDataSize = xiiMemoryUtils::AlignSize<xiiUInt32>(inout_uiAdditionalDataSize, alignof(T));
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
T* xiiVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::Init(xiiUInt8 uiCount, xiiUInt32 uiAlignment, xiiUInt8*& inout_pAdditionalData)
{
  if (uiCount <= Size)
  {
    return m_Embedded;
  }

  inout_pAdditionalData = xiiMemoryUtils::AlignForwards(inout_pAdditionalData, uiAlignment);
  m_Ptr                 = reinterpret_cast<T*>(inout_pAdditionalData);
  inout_pAdditionalData += uiCount * sizeof(T);
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

  T*              pTargetPtr       = Init(out_uiCount, alignof(T), inout_pAdditionalData);
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

XII_ALWAYS_INLINE xiiVisualScriptGraphDescription::DataOffset* xiiVisualScriptGraphDescription::Node::GetInputDataOffsets()
{
  return m_NumInputDataOffsets <= XII_ARRAY_SIZE(m_InputDataOffsets.m_Embedded) ? m_InputDataOffsets.m_Embedded : m_InputDataOffsets.m_Ptr;
}

XII_ALWAYS_INLINE xiiVisualScriptGraphDescription::DataOffset* xiiVisualScriptGraphDescription::Node::GetOutputDataOffsets()
{
  return m_NumOutputDataOffsets <= XII_ARRAY_SIZE(m_OutputDataOffsets.m_Embedded) ? m_OutputDataOffsets.m_Embedded : m_OutputDataOffsets.m_Ptr;
}

// static
template <typename T>
XII_ALWAYS_INLINE constexpr xiiUInt32 xiiVisualScriptGraphDescription::Node::GetUserDataAlignment()
{
  // Ensures at least 8 byte alignment, thus making it compatible between 32 and 64 bit platforms.
  return xiiMath::Max<xiiUInt32>(alignof(T), 8u);
}

template <typename T>
XII_ALWAYS_INLINE const T& xiiVisualScriptGraphDescription::Node::GetUserData() const
{
  XII_ASSERT_DEBUG(m_UserDataByteSize >= sizeof(T), "Invalid data");
  return *reinterpret_cast<const T*>(m_UserDataByteSize <= sizeof(m_UserData.m_Embedded) ? m_UserData.m_Embedded : m_UserData.m_Ptr);
}

template <typename T>
T& xiiVisualScriptGraphDescription::Node::InitUserData(xiiUInt8*& inout_pAdditionalData, xiiUInt32 uiByteSize /*= sizeof(T)*/, xiiUInt32 uiAlignment /*= GetUserDataAlignment<T>()*/)
{
  m_UserDataByteSize              = uiByteSize;
  const xiiUInt32 uiUserDataCount = uiByteSize / sizeof(xiiUInt32);
  XII_ASSERT_DEBUG(uiUserDataCount <= xiiMath::MaxValue<xiiUInt8>(), "User data is too big");
  auto pUserData = m_UserData.Init(static_cast<xiiUInt8>(uiUserDataCount), uiAlignment, inout_pAdditionalData);
  XII_CHECK_ALIGNMENT(pUserData, uiAlignment);
  return *reinterpret_cast<T*>(pUserData);
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE const xiiVisualScriptGraphDescription::Node* xiiVisualScriptGraphDescription::GetNode(xiiUInt32 uiIndex) const
{
  return uiIndex < m_Nodes.GetCount() ? &m_Nodes.GetPtr()[uiIndex] : nullptr;
}

XII_ALWAYS_INLINE bool xiiVisualScriptGraphDescription::IsCoroutine() const
{
  auto entryNodeType = GetNode(0)->m_Type;
  return entryNodeType == xiiVisualScriptNodeDescription::Type::EntryCall_Coroutine || entryNodeType == xiiVisualScriptNodeDescription::Type::MessageHandler_Coroutine;
}

XII_ALWAYS_INLINE const xiiSharedPtr<const xiiVisualScriptDataDescription>& xiiVisualScriptGraphDescription::GetLocalDataDesc() const
{
  return m_pLocalDataDesc;
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
XII_FORCE_INLINE const T& xiiVisualScriptExecutionContext::GetData(DataOffset dataOffset) const
{
  return m_DataStorage[dataOffset.m_uiSource]->GetData<T>(dataOffset);
}

template <typename T>
XII_FORCE_INLINE T& xiiVisualScriptExecutionContext::GetWritableData(DataOffset dataOffset)
{
  XII_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Can't write to constant data");
  return m_DataStorage[dataOffset.m_uiSource]->GetWritableData<T>(dataOffset);
}

template <typename T>
XII_FORCE_INLINE void xiiVisualScriptExecutionContext::SetData(DataOffset dataOffset, const T& value)
{
  XII_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Outputs can't set constant data");
  return m_DataStorage[dataOffset.m_uiSource]->SetData<T>(dataOffset, value);
}

XII_FORCE_INLINE xiiTypedPointer xiiVisualScriptExecutionContext::GetPointerData(DataOffset dataOffset)
{
  XII_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Pointers can't be constant data");
  return m_DataStorage[dataOffset.m_uiSource]->GetPointerData(dataOffset, m_uiExecutionCounter);
}

template <typename T>
XII_FORCE_INLINE void xiiVisualScriptExecutionContext::SetPointerData(DataOffset dataOffset, T ptr, const xiiRTTI* pType)
{
  XII_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Pointers can't be constant data");
  m_DataStorage[dataOffset.m_uiSource]->SetPointerData(dataOffset, ptr, pType, m_uiExecutionCounter);
}

XII_FORCE_INLINE xiiVariant xiiVisualScriptExecutionContext::GetDataAsVariant(DataOffset dataOffset, const xiiRTTI* pExpectedType) const
{
  return m_DataStorage[dataOffset.m_uiSource]->GetDataAsVariant(dataOffset, pExpectedType, m_uiExecutionCounter);
}

XII_FORCE_INLINE void xiiVisualScriptExecutionContext::SetDataFromVariant(DataOffset dataOffset, const xiiVariant& value)
{
  XII_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Outputs can't set constant data");
  return m_DataStorage[dataOffset.m_uiSource]->SetDataFromVariant(dataOffset, value, m_uiExecutionCounter);
}

XII_ALWAYS_INLINE void xiiVisualScriptExecutionContext::SetCurrentCoroutine(xiiScriptCoroutine* pCoroutine)
{
  m_pCurrentCoroutine = pCoroutine;
}

inline xiiTime xiiVisualScriptExecutionContext::GetDeltaTimeSinceLastExecution()
{
  XII_ASSERT_DEBUG(m_pDesc->IsCoroutine(), "Delta time is only valid for coroutines");
  return m_DeltaTimeSinceLastExecution;
}
