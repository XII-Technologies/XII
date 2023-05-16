XII_FORCE_INLINE bool xiiShaderResourceType::IsArray(xiiShaderResourceType::Enum format)
{
  switch (format)
  {
    case xiiShaderResourceType::Texture1DArray:
    case xiiShaderResourceType::Texture2DArray:
    case xiiShaderResourceType::Texture2DMSArray:
    case xiiShaderResourceType::TextureCubeArray:
      return true;
    default:
      return false;
  }
}

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

XII_FORCE_INLINE bool xiiGALShaderCreationDescription::HasByteCodeForStage(xiiGALShaderStage::Enum Stage) const
{
  return m_ByteCodes[Stage] != nullptr && m_ByteCodes[Stage]->IsValid();
}

XII_FORCE_INLINE void xiiGALTextureCreationDescription::SetAsRenderTarget(
  xiiUInt32                   uiWidth,
  xiiUInt32                   uiHeight,
  xiiGALResourceFormat::Enum  format,
  xiiGALMSAASampleCount::Enum sampleCount /*= xiiGALMSAASampleCount::None*/)
{
  m_uiWidth                     = uiWidth;
  m_uiHeight                    = uiHeight;
  m_uiDepth                     = 1;
  m_uiMipLevelCount             = 1;
  m_uiArraySize                 = 1;
  m_SampleCount                 = sampleCount;
  m_Format                      = format;
  m_Type                        = sampleCount != xiiGALMSAASampleCount::None ? xiiGALTextureType::Texture2DArray : xiiGALTextureType::Texture2D;
  m_bAllowShaderResourceView    = true;
  m_bAllowUAV                   = false;
  m_bCreateRenderTarget         = true;
  m_bAllowDynamicMipGeneration  = false;
  m_ResourceAccess.m_bReadBack  = false;
  m_ResourceAccess.m_bImmutable = true;
  m_pExisitingNativeObject      = nullptr;
}

XII_FORCE_INLINE xiiGALVertexAttribute::xiiGALVertexAttribute(
  xiiGALVertexAttributeSemantic::Enum eSemantic,
  xiiGALResourceFormat::Enum          eFormat,
  xiiUInt16                           uiOffset,
  xiiUInt8                            uiVertexBufferSlot,
  bool                                bInstanceData) :
  m_eSemantic(eSemantic), m_eFormat(eFormat), m_uiOffset(uiOffset), m_uiVertexBufferSlot(uiVertexBufferSlot), m_bInstanceData(bInstanceData)
{
}
