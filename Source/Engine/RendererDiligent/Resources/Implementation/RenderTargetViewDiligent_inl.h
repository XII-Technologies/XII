
XII_ALWAYS_INLINE Diligent::ITextureView* xiiGALRenderTargetViewDiligent::GetRenderTargetView()
{
  return m_pRenderTargetView.RawPtr();
}

XII_ALWAYS_INLINE Diligent::ITextureView* xiiGALRenderTargetViewDiligent::GetDepthStencilView()
{
  return m_pDepthStencilView.RawPtr();
}

XII_ALWAYS_INLINE Diligent::ITextureView* xiiGALRenderTargetViewDiligent::GetUnorderedAccessView()
{
  return m_pUnorderedAccessView.RawPtr();
}
