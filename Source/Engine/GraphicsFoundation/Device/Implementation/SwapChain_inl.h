
XII_ALWAYS_INLINE const xiiGALSwapChainCreationDescription& xiiGALSwapChain::GetDescription() const
{
  return m_Description;
}

XII_ALWAYS_INLINE void xiiGALSwapChain::SetPresentMode(xiiEnum<xiiGALPresentMode> presentMode)
{
  if (m_PresentMode != presentMode)
  {
    m_PresentMode = presentMode;
  }
}

XII_ALWAYS_INLINE xiiGALTextureHandle xiiGALSwapChain::GetBackBufferTexture() const
{
  return m_hBackBufferTexture;
}

XII_ALWAYS_INLINE xiiSizeU32 xiiGALSwapChain::GetCurrentSize() const
{
  return m_CurrentSize;
}

XII_ALWAYS_INLINE xiiEnum<xiiGALPresentMode> xiiGALSwapChain::GetPresentMode() const
{
  return m_PresentMode;
}
