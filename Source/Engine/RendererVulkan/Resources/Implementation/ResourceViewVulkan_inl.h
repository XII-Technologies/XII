XII_ALWAYS_INLINE const vk::DescriptorImageInfo& xiiGALResourceViewVulkan::GetImageInfo(bool bIsArray) const
{
  XII_ASSERT_DEBUG((bIsArray ? m_resourceImageInfoArray : m_resourceImageInfo).imageView, "View does not support bIsArray: {}", bIsArray);
  return bIsArray ? m_resourceImageInfoArray : m_resourceImageInfo;
}

XII_ALWAYS_INLINE vk::ImageSubresourceRange xiiGALResourceViewVulkan::GetRange() const
{
  return m_range;
}

XII_ALWAYS_INLINE const vk::BufferView& xiiGALResourceViewVulkan::GetBufferView() const
{
  return m_bufferView;
}
