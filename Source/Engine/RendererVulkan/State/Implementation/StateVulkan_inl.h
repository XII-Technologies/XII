
XII_ALWAYS_INLINE const vk::PipelineColorBlendStateCreateInfo* xiiGALBlendStateVulkan::GetBlendState() const
{
  return &m_blendState;
}

XII_ALWAYS_INLINE const vk::PipelineDepthStencilStateCreateInfo* xiiGALDepthStencilStateVulkan::GetDepthStencilState() const
{
  return &m_depthStencilState;
}

XII_ALWAYS_INLINE const vk::PipelineRasterizationStateCreateInfo* xiiGALRasterizerStateVulkan::GetRasterizerState() const
{
  return &m_rasterizerState;
}

XII_ALWAYS_INLINE const vk::DescriptorImageInfo& xiiGALSamplerStateVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}
