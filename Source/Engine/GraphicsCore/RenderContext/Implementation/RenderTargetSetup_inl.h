
XII_ALWAYS_INLINE xiiUInt8 xiiGALRenderTargetSetup::GetRenderTargetCount() const
{
  return m_uiRTCount;
}

XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTextureView> xiiGALRenderTargetSetup::GetRenderTarget(xiiUInt8 uiIndex) const
{
  XII_ASSERT_DEBUG(uiIndex < m_uiRTCount, "Render target index out of range");

  return m_pRTs[uiIndex];
}

XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTextureView> xiiGALRenderTargetSetup::GetDepthStencilTarget() const
{
  return m_pDSTarget;
}
