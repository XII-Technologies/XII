

XII_ALWAYS_INLINE ID3D11RenderTargetView* xiiGALRenderTargetViewDX11::GetRenderTargetView() const
{
  return m_pRenderTargetView;
}

XII_ALWAYS_INLINE ID3D11DepthStencilView* xiiGALRenderTargetViewDX11::GetDepthStencilView() const
{
  return m_pDepthStencilView;
}

XII_ALWAYS_INLINE ID3D11UnorderedAccessView* xiiGALRenderTargetViewDX11::GetUnorderedAccessView() const
{
  return m_pUnorderedAccessView;
}