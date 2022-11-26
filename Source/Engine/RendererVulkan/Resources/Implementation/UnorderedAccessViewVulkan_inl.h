XII_ALWAYS_INLINE const vk::DescriptorImageInfo& xiiGALUnorderedAccessViewVulkan::GetImageInfo() const
{
  return m_resourceImageInfo;
}

XII_ALWAYS_INLINE vk::ImageSubresourceRange xiiGALUnorderedAccessViewVulkan::GetRange() const
{
  return m_range;
}
