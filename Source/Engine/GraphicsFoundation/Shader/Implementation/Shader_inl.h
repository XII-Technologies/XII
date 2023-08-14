
XII_FORCE_INLINE xiiGALShaderCreationDescription::xiiGALShaderCreationDescription() :
  xiiHashableStruct()
{
}

XII_FORCE_INLINE xiiGALShaderCreationDescription::~xiiGALShaderCreationDescription()
{
  for (xiiUInt32 i = 0; i < m_ByteCodes.GetCount(); ++i)
  {
    xiiGALShaderByteCode* pByteCode = m_ByteCodes[i];
    m_ByteCodes[i]                  = nullptr;

    if (pByteCode != nullptr && pByteCode->GetRefCount() == 0)
    {
      XII_DEFAULT_DELETE(pByteCode);
    }
  }
}

XII_FORCE_INLINE bool xiiGALShaderCreationDescription::HasByteCodeForStage(xiiGALShaderStage::Enum stage) const
{
  return m_ByteCodes[stage] != nullptr && m_ByteCodes[stage]->IsValid();
}
