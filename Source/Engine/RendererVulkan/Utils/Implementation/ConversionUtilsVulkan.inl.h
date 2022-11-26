#include <RendererFoundation/Resources/ResourceFormats.h>

namespace
{
  bool IsArrayViewInternal(const xiiGALTextureCreationDescription& texDesc, const xiiGALResourceViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
  bool IsArrayViewInternal(const xiiGALTextureCreationDescription& texDesc, const xiiGALUnorderedAccessViewCreationDescription& viewDesc)
  {
    return texDesc.m_uiArraySize > 1 || viewDesc.m_uiFirstArraySlice > 0;
  }
} // namespace

XII_ALWAYS_INLINE vk::SampleCountFlagBits xiiConversionUtilsVulkan::GetSamples(xiiEnum<xiiGALMSAASampleCount> samples)
{
  switch (samples)
  {
    case xiiGALMSAASampleCount::None:
      return vk::SampleCountFlagBits::e1;
    case xiiGALMSAASampleCount::TwoSamples:
      return vk::SampleCountFlagBits::e2;
    case xiiGALMSAASampleCount::FourSamples:
      return vk::SampleCountFlagBits::e4;
    case xiiGALMSAASampleCount::EightSamples:
      return vk::SampleCountFlagBits::e8;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return vk::SampleCountFlagBits::e1;
  }
}

XII_ALWAYS_INLINE vk::PresentModeKHR xiiConversionUtilsVulkan::GetPresentMode(xiiEnum<xiiGALPresentMode> presentMode, const xiiDynamicArray<vk::PresentModeKHR>& supportedModes)
{
  switch (presentMode)
  {
    case xiiGALPresentMode::Immediate:
    {
      if (supportedModes.Contains(vk::PresentModeKHR::eImmediate))
        return vk::PresentModeKHR::eImmediate;
      else if (supportedModes.Contains(vk::PresentModeKHR::eMailbox))
        return vk::PresentModeKHR::eMailbox;
      else
        return vk::PresentModeKHR::eFifo;
    }
    case xiiGALPresentMode::VSync:
      return vk::PresentModeKHR::eFifo; // FIFO must be supported according to the standard.
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return vk::PresentModeKHR::eFifo;
  }
}

XII_ALWAYS_INLINE vk::ImageSubresourceRange xiiConversionUtilsVulkan::GetSubresourceRange(const xiiGALTextureCreationDescription& texDesc, const xiiGALRenderTargetViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange  range;
  xiiGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == xiiGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask                      = xiiGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  range.setBaseMipLevel(viewDesc.m_uiMipLevel).setLevelCount(1).setBaseArrayLayer(viewDesc.m_uiFirstSlice).setLayerCount(viewDesc.m_uiSliceCount);
  return range;
}

XII_ALWAYS_INLINE vk::ImageSubresourceRange xiiConversionUtilsVulkan::GetSubresourceRange(const xiiGALTextureCreationDescription& texDesc, const xiiGALResourceViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  xiiGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == xiiGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask                      = xiiGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == xiiGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }
  range.baseMipLevel = viewDesc.m_uiMostDetailedMipLevel;
  range.levelCount   = xiiMath::Min(viewDesc.m_uiMipLevelsToUse, texDesc.m_uiMipLevelCount - range.baseMipLevel);

  switch (texDesc.m_Type)
  {
    case xiiGALTextureType::Texture2D:
    case xiiGALTextureType::Texture2DProxy:
      range.layerCount     = viewDesc.m_uiArraySize;
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case xiiGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      range.layerCount     = viewDesc.m_uiArraySize * 6;
      break;
    case xiiGALTextureType::Texture3D:
      range.layerCount = 1;
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}


XII_ALWAYS_INLINE vk::ImageSubresourceRange xiiConversionUtilsVulkan::GetSubresourceRange(const xiiGALTextureCreationDescription& texDesc, const xiiGALUnorderedAccessViewCreationDescription& viewDesc)
{
  vk::ImageSubresourceRange range;

  const bool bIsArrayView = IsArrayViewInternal(texDesc, viewDesc);

  xiiGALResourceFormat::Enum viewFormat = viewDesc.m_OverrideViewFormat == xiiGALResourceFormat::Invalid ? texDesc.m_Format : viewDesc.m_OverrideViewFormat;
  range.aspectMask                      = xiiGALResourceFormat::IsDepthFormat(viewFormat) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;
  if (viewFormat == xiiGALResourceFormat::D24S8)
  {
    range.aspectMask |= vk::ImageAspectFlagBits::eStencil;
  }

  range.baseMipLevel = viewDesc.m_uiMipLevelToUse;
  range.levelCount   = 1;
  range.layerCount   = viewDesc.m_uiArraySize;

  switch (texDesc.m_Type)
  {
    case xiiGALTextureType::Texture2D:
    case xiiGALTextureType::Texture2DProxy:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case xiiGALTextureType::TextureCube:
      range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      break;
    case xiiGALTextureType::Texture3D:
      if (bIsArrayView)
      {
        XII_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        range.baseArrayLayer = viewDesc.m_uiFirstArraySlice;
      }
      break;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }
  return range;
}

XII_ALWAYS_INLINE vk::ImageSubresourceRange xiiConversionUtilsVulkan::GetSubresourceRange(
  const vk::ImageSubresourceLayers& layers)
{
  vk::ImageSubresourceRange range;
  range.aspectMask     = layers.aspectMask;
  range.baseMipLevel   = layers.mipLevel;
  range.levelCount     = 1;
  range.baseArrayLayer = layers.baseArrayLayer;
  range.layerCount     = layers.layerCount;
  return range;
}

XII_ALWAYS_INLINE vk::ImageViewType xiiConversionUtilsVulkan::GetImageViewType(xiiEnum<xiiGALTextureType> texType, bool bIsArrayView)
{
  switch (texType)
  {
    case xiiGALTextureType::Texture2D:
    case xiiGALTextureType::Texture2DProxy:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::e2D;
      }
      else
      {
        return vk::ImageViewType::e2DArray;
      }
    case xiiGALTextureType::TextureCube:
      if (!bIsArrayView)
      {
        return vk::ImageViewType::eCube;
      }
      else
      {
        return vk::ImageViewType::eCubeArray;
      }
    case xiiGALTextureType::Texture3D:
      return vk::ImageViewType::e3D;

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return vk::ImageViewType::e1D;
  }
}

XII_ALWAYS_INLINE bool xiiConversionUtilsVulkan::IsDepthFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eD16Unorm:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

XII_ALWAYS_INLINE bool xiiConversionUtilsVulkan::IsStencilFormat(vk::Format format)
{
  switch (format)
  {
    case vk::Format::eS8Uint:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
      return true;
    default:
      return false;
  }
}

XII_ALWAYS_INLINE vk::PrimitiveTopology xiiConversionUtilsVulkan::GetPrimitiveTopology(xiiEnum<xiiGALPrimitiveTopology> topology)
{
  switch (topology)
  {
    case xiiGALPrimitiveTopology::Points:
      return vk::PrimitiveTopology::ePointList;
    case xiiGALPrimitiveTopology::Lines:
      return vk::PrimitiveTopology::eLineList;
    case xiiGALPrimitiveTopology::Triangles:
      return vk::PrimitiveTopology::eTriangleList;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      return vk::PrimitiveTopology::ePointList;
  }
}

XII_ALWAYS_INLINE vk::ShaderStageFlagBits xiiConversionUtilsVulkan::GetShaderStage(xiiGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case xiiGALShaderStage::VertexShader:
      return vk::ShaderStageFlagBits::eVertex;
    case xiiGALShaderStage::HullShader:
      return vk::ShaderStageFlagBits::eTessellationControl;
    case xiiGALShaderStage::DomainShader:
      return vk::ShaderStageFlagBits::eTessellationEvaluation;
    case xiiGALShaderStage::GeometryShader:
      return vk::ShaderStageFlagBits::eGeometry;
    case xiiGALShaderStage::PixelShader:
      return vk::ShaderStageFlagBits::eFragment;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case xiiGALShaderStage::ComputeShader:
      return vk::ShaderStageFlagBits::eCompute;
  }
}

XII_ALWAYS_INLINE vk::PipelineStageFlags xiiConversionUtilsVulkan::GetPipelineStage(xiiGALShaderStage::Enum stage)
{
  switch (stage)
  {
    case xiiGALShaderStage::VertexShader:
      return vk::PipelineStageFlagBits::eVertexShader;
    case xiiGALShaderStage::HullShader:
      return vk::PipelineStageFlagBits::eTessellationControlShader;
    case xiiGALShaderStage::DomainShader:
      return vk::PipelineStageFlagBits::eTessellationEvaluationShader;
    case xiiGALShaderStage::GeometryShader:
      return vk::PipelineStageFlagBits::eGeometryShader;
    case xiiGALShaderStage::PixelShader:
      return vk::PipelineStageFlagBits::eFragmentShader;
    default:
      XII_ASSERT_NOT_IMPLEMENTED;
      [[fallthrough]];
    case xiiGALShaderStage::ComputeShader:
      return vk::PipelineStageFlagBits::eComputeShader;
  }
}

XII_ALWAYS_INLINE vk::PipelineStageFlags xiiConversionUtilsVulkan::GetPipelineStage(vk::ShaderStageFlags flags)
{
  vk::PipelineStageFlags res;
  if (flags & vk::ShaderStageFlagBits::eVertex)
    res |= vk::PipelineStageFlagBits::eVertexShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationControl)
    res |= vk::PipelineStageFlagBits::eTessellationControlShader;
  if (flags & vk::ShaderStageFlagBits::eTessellationEvaluation)
    res |= vk::PipelineStageFlagBits::eTessellationEvaluationShader;
  if (flags & vk::ShaderStageFlagBits::eGeometry)
    res |= vk::PipelineStageFlagBits::eGeometryShader;
  if (flags & vk::ShaderStageFlagBits::eFragment)
    res |= vk::PipelineStageFlagBits::eFragmentShader;
  if (flags & vk::ShaderStageFlagBits::eCompute)
    res |= vk::PipelineStageFlagBits::eComputeShader;

  return res;
}
