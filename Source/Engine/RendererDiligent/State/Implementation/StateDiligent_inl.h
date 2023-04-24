
XII_ALWAYS_INLINE const Diligent::BlendStateDesc* xiiGALBlendStateDiligent::GetBlendStateDesc() const
{
  return &m_BlendStateDesc;
}

XII_ALWAYS_INLINE const Diligent::DepthStencilStateDesc* xiiGALDepthStencilStateDiligent::GetDepthStencilStateDesc() const
{
  return &m_DepthStencilStateDesc;
}

XII_ALWAYS_INLINE const Diligent::RasterizerStateDesc* xiiGALRasterizerStateDiligent::GetRasterizerStateDesc() const
{
  return &m_RasterizerStateDesc;
}

XII_ALWAYS_INLINE Diligent::ISampler* xiiGALSamplerStateDiligent::GetSamplerState()
{
  return m_pSamplerState;
}
