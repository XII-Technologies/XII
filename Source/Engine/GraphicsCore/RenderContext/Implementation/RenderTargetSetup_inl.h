
XII_ALWAYS_INLINE xiiUInt8 xiiGALRenderTargetSetup::GetRenderTargetCount() const
{
  return m_uiRTCount;
}

XII_ALWAYS_INLINE xiiGALTextureViewHandle xiiGALRenderTargetSetup::GetRenderTarget(xiiUInt8 uiIndex) const
{
  XII_ASSERT_DEBUG(uiIndex < m_uiRTCount, "Render target index out of range");

  return m_hRTs[uiIndex];
}

XII_ALWAYS_INLINE xiiGALTextureViewHandle xiiGALRenderTargetSetup::GetDepthStencilTarget() const
{
  return m_hDSTarget;
}
