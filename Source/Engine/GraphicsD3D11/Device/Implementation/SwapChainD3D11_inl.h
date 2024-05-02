
XII_ALWAYS_INLINE void xiiGALSwapChainD3D11::SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode)
{
  if (m_pSwapChain)
  {
    // If we are already in fullscreen mode, we need to switch to windowed mode first,
    // because a swap chain must be in windowed mode when it is released.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#Destroying
    if (m_FullScreenMode.m_bIsFullScreen)
    {
      m_pSwapChain->SetFullscreenState(FALSE, nullptr);
    }

    m_FullScreenMode.m_bIsFullScreen            = true;
    m_FullScreenMode.m_uiRefreshRateNumerator   = displayMode.m_uiRefreshRateNumerator;
    m_FullScreenMode.m_uiRefreshRateDenominator = displayMode.m_uiRefreshRateDenominator;
    m_FullScreenMode.m_ScalingMode              = displayMode.m_ScalingMode;
    m_FullScreenMode.m_ScanLineOrder            = displayMode.m_ScanLineOrder;

    m_Description.m_Resolution = displayMode.m_Resolution;
    if (displayMode.m_TextureFormat != xiiGALTextureFormat::Unknown)
    {
      m_Description.m_ColorBufferFormat = displayMode.m_TextureFormat;
    }

    UpdateSwapChain(true).AssertSuccess();
  }
}

XII_ALWAYS_INLINE void xiiGALSwapChainD3D11::SetWindowedMode()
{
}

XII_ALWAYS_INLINE IDXGISwapChain1* xiiGALSwapChainD3D11::GetSwapChain() const
{
  return m_pSwapChain;
}
