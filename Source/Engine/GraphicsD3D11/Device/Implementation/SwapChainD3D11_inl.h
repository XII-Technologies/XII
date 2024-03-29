
XII_ALWAYS_INLINE void xiiGALSwapChainD3D11::SetFullScreenMode(const xiiGALDisplayModeDescription& displayMode)
{
  Diligent::DisplayModeAttribs displayModeAttribs;
  displayModeAttribs.Width                  = displayMode.m_Resolution.width;
  displayModeAttribs.Height                 = displayMode.m_Resolution.height;
  displayModeAttribs.Format                 = xiiDiligentTypeConversions::GetTextureFormat(displayMode.m_TextureFormat);
  displayModeAttribs.RefreshRateNumerator   = displayMode.m_uiRefreshRateNumerator;
  displayModeAttribs.RefreshRateDenominator = displayMode.m_uiRefreshRateDenominator;
  displayModeAttribs.Scaling                = xiiDiligentTypeConversions::GetScalingMode(displayMode.m_ScalingMode);
  displayModeAttribs.ScanlineOrder          = xiiDiligentTypeConversions::GetScanLineOrder(displayMode.m_ScanLineOrder);

  m_pSwapChain->SetFullscreenMode(displayModeAttribs);
}

XII_ALWAYS_INLINE void xiiGALSwapChainD3D11::SetWindowedMode()
{
  m_pSwapChain->SetWindowedMode();
}

XII_ALWAYS_INLINE void xiiGALSwapChainD3D11::SetMaximumFrameLatency(xiiUInt32 uiMaxLatency)
{
  m_pSwapChain->SetMaximumFrameLatency(uiMaxLatency);
}

XII_ALWAYS_INLINE Diligent::ISwapChain* xiiGALSwapChainD3D11::GetSwapChain() const
{
  return m_pSwapChain;
}
