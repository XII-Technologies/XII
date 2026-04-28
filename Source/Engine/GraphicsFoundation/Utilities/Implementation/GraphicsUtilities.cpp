/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>

xiiBitflags<xiiGALBindFlags> xiiGALGraphicsUtilities::SwapChainUsageFlagsToBindFlags(xiiBitflags<xiiGALSwapChainUsageFlags> swapChainUsageFlags)
{
  xiiBitflags<xiiGALBindFlags> bindFlags = xiiGALBindFlags::None;

  if (swapChainUsageFlags.IsSet(xiiGALSwapChainUsageFlags::RenderTarget))
    bindFlags |= xiiGALBindFlags::RenderTarget;
  if (swapChainUsageFlags.IsSet(xiiGALSwapChainUsageFlags::ShaderResource))
    bindFlags |= xiiGALBindFlags::ShaderResource;
  if (swapChainUsageFlags.IsSet(xiiGALSwapChainUsageFlags::InputAttachment))
    bindFlags |= xiiGALBindFlags::InputAttachment;

  // No special bind flag is needed for xiiGALSwapChainUsageFlags::CopySource.

  return bindFlags;
}

xiiBitflags<xiiGALPipelineResourceFlags> xiiGALGraphicsUtilities::GetValidPipelineResourceFlags(xiiEnum<xiiGALShaderResourceType> type)
{
  xiiBitflags<xiiGALPipelineResourceFlags> pipelineResourceFlags = xiiGALPipelineResourceFlags::None;

  switch (type)
  {
    case xiiGALShaderResourceType::ConstantBuffer:
      pipelineResourceFlags |= xiiGALPipelineResourceFlags::NoDynamicBuffers | xiiGALPipelineResourceFlags::RuntimeArray;
      break;
    case xiiGALShaderResourceType::TextureSRV:
    case xiiGALShaderResourceType::TextureAndSampler:
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

xiiGALSamplerCreationDescription xiiGALGraphicsUtilities::GetDefaultSamplerDescription() noexcept
{
  return xiiGALSamplerCreationDescription{
    .m_MinFilter          = xiiGALFilterType::Linear,
    .m_MagFilter          = xiiGALFilterType::Linear,
    .m_MipFilter          = xiiGALFilterType::Linear,
    .m_AddressU           = xiiGALTextureAddressMode::Wrap,
    .m_AddressV           = xiiGALTextureAddressMode::Wrap,
    .m_AddressW           = xiiGALTextureAddressMode::Wrap,
    .m_Flags              = xiiGALSamplerFlags::None,
    .m_bUnormalizedCoords = false,
    .m_fMipLODBias        = 0.0f,
    .m_uiMaxAnisotropy    = 4,
    .m_ComparisonFunction = xiiGALComparisonFunction::Never,
    .m_BorderColor        = xiiColor::Black,
    .m_fMinLOD            = -1.0f,
    .m_fMaxLOD            = 4200.0f,
  };
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Utilities_Implementation_GraphicsUtilities);
