
XII_ALWAYS_INLINE const xiiExpressionByteCode::StorageType* xiiExpressionByteCode::GetByteCode() const
{
  return m_ByteCode.GetData();
}

XII_ALWAYS_INLINE const xiiExpressionByteCode::StorageType* xiiExpressionByteCode::GetByteCodeEnd() const
{
  return m_ByteCode.GetData() + m_ByteCode.GetCount();
}

XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetNumInstructions() const
{
  return m_uiNumInstructions;
}

XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetNumTempRegisters() const
{
  return m_uiNumTempRegisters;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiHashedString> xiiExpressionByteCode::GetInputs() const
{
  return m_Inputs;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiHashedString> xiiExpressionByteCode::GetOutputs() const
{
  return m_Outputs;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiHashedString> xiiExpressionByteCode::GetFunctions() const
{
  return m_Functions;
}

// static
XII_ALWAYS_INLINE xiiExpressionByteCode::OpCode::Enum xiiExpressionByteCode::GetOpCode(const StorageType*& pByteCode)
{
  xiiUInt32 uiOpCode = *pByteCode;
  ++pByteCode;
  return static_cast<OpCode::Enum>(uiOpCode);
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetRegisterIndex(const StorageType*& pByteCode, xiiUInt32 uiNumRegisters)
{
  xiiUInt32 uiIndex = *pByteCode * uiNumRegisters;
  ++pByteCode;
  return uiIndex;
}

// static
XII_ALWAYS_INLINE xiiSimdVec4f xiiExpressionByteCode::GetConstant(const StorageType*& pByteCode)
{
  float c = *reinterpret_cast<const float*>(pByteCode);
  ++pByteCode;
  return xiiSimdVec4f(c);
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetFunctionIndex(const StorageType*& pByteCode)
{
  xiiUInt32 uiIndex = *pByteCode;
  ++pByteCode;
  return uiIndex;
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetFunctionArgCount(const StorageType*& pByteCode)
{
  xiiUInt32 uiArgCount = *pByteCode;
  ++pByteCode;
  return uiArgCount;
}
