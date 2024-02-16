
const void* xiiGALShaderByteCode::GetByteCode() const
{
  if (m_ByteCode.IsEmpty())
    return nullptr;

  return m_ByteCode.GetData();
}

xiiUInt32 xiiGALShaderByteCode::GetSize() const
{
  return m_ByteCode.GetCount();
}

bool xiiGALShaderByteCode::IsValid() const
{
  return !m_ByteCode.IsEmpty();
}

XII_ALWAYS_INLINE const xiiGALShaderResourceDescription* xiiGALShaderByteCode::GetDescription(const xiiTempHashedString& sName) const
{
  for (const auto& binding : m_ShaderResourceBindings)
  {
    if (binding.m_sName == sName)
    {
      return &binding;
    }
  }
  return nullptr;
}
