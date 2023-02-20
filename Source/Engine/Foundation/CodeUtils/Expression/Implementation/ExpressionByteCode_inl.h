
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

XII_ALWAYS_INLINE xiiArrayPtr<const xiiExpression::StreamDesc> xiiExpressionByteCode::GetInputs() const
{
  return m_Inputs;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiExpression::StreamDesc> xiiExpressionByteCode::GetOutputs() const
{
  return m_Outputs;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiExpression::FunctionDesc> xiiExpressionByteCode::GetFunctions() const
{
  return m_Functions;
}

// static
XII_ALWAYS_INLINE xiiExpressionByteCode::OpCode::Enum xiiExpressionByteCode::GetOpCode(const StorageType*& pByteCode)
{
  xiiUInt32 uiOpCode = *pByteCode;
  ++pByteCode;
  return static_cast<OpCode::Enum>((uiOpCode >= 0 && uiOpCode < OpCode::Count) ? uiOpCode : 0);
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetRegisterIndex(const StorageType*& pByteCode)
{
  xiiUInt32 uiIndex = *pByteCode;
  ++pByteCode;
  return uiIndex;
}

// static
XII_ALWAYS_INLINE xiiExpression::Register xiiExpressionByteCode::GetConstant(const StorageType*& pByteCode)
{
  xiiExpression::Register r;
  r.i = xiiSimdVec4i(*pByteCode);
  ++pByteCode;
  return r;
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
