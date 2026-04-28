/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE const xiiExpressionByteCode::StorageType* xiiExpressionByteCode::GetByteCodeStart() const
{
  return m_pByteCode;
}

XII_ALWAYS_INLINE const xiiExpressionByteCode::StorageType* xiiExpressionByteCode::GetByteCodeEnd() const
{
  return m_pByteCode + m_uiByteCodeCount;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiExpressionByteCode::StorageType> xiiExpressionByteCode::GetByteCode() const
{
  return xiiMakeArrayPtr(m_pByteCode, m_uiByteCodeCount);
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
  return xiiMakeArrayPtr(m_pInputs, m_uiNumInputs);
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiExpression::StreamDesc> xiiExpressionByteCode::GetOutputs() const
{
  return xiiMakeArrayPtr(m_pOutputs, m_uiNumOutputs);
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiExpression::FunctionDesc> xiiExpressionByteCode::GetFunctions() const
{
  return xiiMakeArrayPtr(m_pFunctions, m_uiNumFunctions);
}

// static
XII_ALWAYS_INLINE xiiExpressionByteCode::OpCode::Enum xiiExpressionByteCode::GetOpCode(const StorageType*& ref_pByteCode)
{
  xiiUInt32 uiOpCode = *ref_pByteCode;
  ++ref_pByteCode;
  return static_cast<OpCode::Enum>((uiOpCode >= 0 && uiOpCode < OpCode::Count) ? uiOpCode : 0);
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetRegisterIndex(const StorageType*& ref_pByteCode)
{
  xiiUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
XII_ALWAYS_INLINE xiiExpression::Register xiiExpressionByteCode::GetConstant(const StorageType*& ref_pByteCode)
{
  xiiExpression::Register r;
  r.i = xiiSimdVec4i(*ref_pByteCode);
  ++ref_pByteCode;
  return r;
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetFunctionIndex(const StorageType*& ref_pByteCode)
{
  xiiUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
XII_ALWAYS_INLINE xiiUInt32 xiiExpressionByteCode::GetFunctionArgCount(const StorageType*& ref_pByteCode)
{
  xiiUInt32 uiArgCount = *ref_pByteCode;
  ++ref_pByteCode;
  return uiArgCount;
}
