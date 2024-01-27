
XII_ALWAYS_INLINE bool xiiGALGraphicsUtilities::IsIdentityComponentMapping(const xiiGALTextureComponentMapping& mapping)
{
  return ((mapping.m_R == xiiGALTextureComponentSwizzle::Identity || mapping.m_R == xiiGALTextureComponentSwizzle::R) &&
          (mapping.m_G == xiiGALTextureComponentSwizzle::Identity || mapping.m_G == xiiGALTextureComponentSwizzle::G) &&
          (mapping.m_B == xiiGALTextureComponentSwizzle::Identity || mapping.m_B == xiiGALTextureComponentSwizzle::B) &&
          (mapping.m_A == xiiGALTextureComponentSwizzle::Identity || mapping.m_A == xiiGALTextureComponentSwizzle::A));
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALPipelineResourceFlags> xiiGALGraphicsUtilities::GetValidPipelineResourceFlags(xiiEnum<xiiGALShaderResourceType> type)
{
  xiiBitflags<xiiGALPipelineResourceFlags> pipelineResourceFlags = xiiGALPipelineResourceFlags::None;

  switch (type)
  {
    case xiiGALShaderResourceType::ConstantBuffer:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::NoDynamicBuffers | xiiGALPipelineResourceFlags::RuntimeArray;
      break;
    case xiiGALShaderResourceType::TextureSRV:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::CombinedSampler | xiiGALPipelineResourceFlags::RuntimeArray;
      break;
    case xiiGALShaderResourceType::BufferSRV:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::NoDynamicBuffers | xiiGALPipelineResourceFlags::Formattedbuffer | xiiGALPipelineResourceFlags::RuntimeArray;
      break;
    case xiiGALShaderResourceType::TextureUAV:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::RuntimeArray;
      break;
    case xiiGALShaderResourceType::BufferUAV:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::NoDynamicBuffers | xiiGALPipelineResourceFlags::Formattedbuffer | xiiGALPipelineResourceFlags::RuntimeArray;
      break;
    case xiiGALShaderResourceType::Sampler:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::RuntimeArray;
      break;
    case xiiGALShaderResourceType::InputAttachment:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::GeneralInputAttachment;
      break;
    case xiiGALShaderResourceType::AccelerationStructure:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::RuntimeArray;
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return pipelineResourceFlags;
}
