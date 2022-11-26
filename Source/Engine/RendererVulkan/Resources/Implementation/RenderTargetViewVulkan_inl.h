

XII_ALWAYS_INLINE vk::ImageView xiiGALRenderTargetViewVulkan::GetImageView() const
{
  return m_imageView;
}

XII_ALWAYS_INLINE bool xiiGALRenderTargetViewVulkan::IsFullRange() const
{
  return m_bfullRange;
}

XII_ALWAYS_INLINE vk::ImageSubresourceRange xiiGALRenderTargetViewVulkan::GetRange() const
{
  return m_range;
}
