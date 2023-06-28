
template <typename T>
XII_FORCE_INLINE const T& xiiVisualScriptInstance::GetData(DataOffset dataOffset) const
{
  if (dataOffset.m_uiIsConstant)
  {
    return m_pConstantDataStorage->GetData<T>(dataOffset);
  }

  return m_pVariableDataStorage->GetData<T>(dataOffset);
}

template <typename T>
XII_FORCE_INLINE T& xiiVisualScriptInstance::GetWritableData(DataOffset dataOffset)
{
  XII_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Can't write to constant data");
  return m_pVariableDataStorage->GetWritableData<T>(dataOffset);
}

template <typename T>
XII_FORCE_INLINE void xiiVisualScriptInstance::SetData(DataOffset dataOffset, const T& value)
{
  XII_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Outputs can't set constant data");
  return m_pVariableDataStorage->SetData<T>(dataOffset, value);
}

XII_FORCE_INLINE xiiTypedPointer xiiVisualScriptInstance::GetPointerData(DataOffset dataOffset)
{
  XII_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Pointers can't be constant data");
  return m_pVariableDataStorage->GetPointerData(dataOffset, m_uiExecutionCounter);
}

template <typename T>
XII_FORCE_INLINE void xiiVisualScriptInstance::SetPointerData(DataOffset dataOffset, T ptr, const xiiRTTI* pType)
{
  XII_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Pointers can't be constant data");
  m_pVariableDataStorage->SetPointerData(dataOffset, ptr, pType, m_uiExecutionCounter);
}

XII_FORCE_INLINE xiiVariant xiiVisualScriptInstance::GetDataAsVariant(DataOffset dataOffset, xiiVariantType::Enum expectedType) const
{
  if (dataOffset.m_uiIsConstant)
  {
    return m_pConstantDataStorage->GetDataAsVariant(dataOffset, expectedType, m_uiExecutionCounter);
  }

  return m_pVariableDataStorage->GetDataAsVariant(dataOffset, expectedType, m_uiExecutionCounter);
}

XII_FORCE_INLINE void xiiVisualScriptInstance::SetDataFromVariant(DataOffset dataOffset, const xiiVariant& value)
{
  XII_ASSERT_DEBUG(dataOffset.m_uiIsConstant == 0, "Outputs can't set constant data");
  return m_pVariableDataStorage->SetDataFromVariant(dataOffset, value, m_uiExecutionCounter);
}
