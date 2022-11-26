

const void* xiiGALShaderByteCode::GetByteCode() const
{
  if (m_Source.IsEmpty())
    return nullptr;

  return &m_Source[0];
}

xiiUInt32 xiiGALShaderByteCode::GetSize() const
{
  return m_Source.GetCount();
}

bool xiiGALShaderByteCode::IsValid() const
{
  return !m_Source.IsEmpty();
}
