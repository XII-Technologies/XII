
XII_ALWAYS_INLINE const xiiGALRenderTargets& xiiGALSwapChain::GetRenderTargets() const
{
  return m_RenderTargets;
}

XII_ALWAYS_INLINE xiiGALTextureHandle xiiGALSwapChain::GetBackBufferTexture() const
{
  return m_RenderTargets.m_hRTs[0];
}

XII_ALWAYS_INLINE xiiSizeU32 xiiGALSwapChain::GetCurrentSize() const
{
  return m_CurrentSize;
}
