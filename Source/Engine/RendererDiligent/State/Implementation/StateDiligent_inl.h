
XII_ALWAYS_INLINE const Diligent::BlendStateDesc* xiiGALBlendStateDiligent::GetBlendStateDesc() const
{
  return &m_BlendState;
}

XII_ALWAYS_INLINE const Diligent::DepthStencilStateDesc* xiiGALDepthStencilStateDiligent::GetDepthStencilStateDesc() const
{
  return &m_DepthStencilState;
}

XII_ALWAYS_INLINE const Diligent::RasterizerStateDesc* xiiGALRasterizerStateDiligent::GetRasterizerStateDesc() const
{
  return &m_RasterizerState;
}

XII_ALWAYS_INLINE Diligent::ISampler* xiiGALSamplerStateDiligent::GetSamplerState()
{
  return m_pSamplerState.RawPtr();
}
