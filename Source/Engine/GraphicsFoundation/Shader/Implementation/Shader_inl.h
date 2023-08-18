
XII_FORCE_INLINE xiiGALShaderCreationDescription::xiiGALShaderCreationDescription() :
  xiiHashableStruct()
{
}

XII_FORCE_INLINE xiiGALShaderCreationDescription::~xiiGALShaderCreationDescription()
{
  for (xiiUInt32 i = 0; i < xiiGALShaderStage::ENUM_COUNT; ++i)
  {
    xiiGALShaderByteCode* pByteCode = m_ByteCodes[i];
    m_ByteCodes[i]                  = nullptr;

    if (pByteCode != nullptr && pByteCode->GetRefCount() == 0)
    {
      XII_DEFAULT_DELETE(pByteCode);
    }
  }
}

XII_FORCE_INLINE bool xiiGALShaderCreationDescription::HasByteCodeForStage(xiiBitflags<xiiGALShaderStage> stage) const
{
  if (stage == xiiGALShaderStage::Unknown)
    return false;

  const xiiUInt32 uiStageIndex = xiiGALShaderStage::GetStageIndex(stage);
  return m_ByteCodes[uiStageIndex] != nullptr && m_ByteCodes[uiStageIndex]->IsValid();
}
