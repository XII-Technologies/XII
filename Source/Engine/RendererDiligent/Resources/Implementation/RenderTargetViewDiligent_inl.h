
XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::ITextureView>& xiiGALRenderTargetViewDiligent::GetRenderTargetView()
{
  return m_pRenderTargetView;
}

XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::ITextureView>& xiiGALRenderTargetViewDiligent::GetDepthStencilView()
{
  return m_pDepthStencilView;
}

XII_ALWAYS_INLINE Diligent::RefCntAutoPtr<Diligent::ITextureView>& xiiGALRenderTargetViewDiligent::GetUnorderedAccessView()
{
  return m_pUnorderedAccessView;
}
