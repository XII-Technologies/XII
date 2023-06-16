
XII_ALWAYS_INLINE Diligent::IDeviceObject* xiiGALUnorderedAccessViewDiligent::GetResourceView()
{
  if (m_pUnorderedAccessTextureView)
    return m_pUnorderedAccessTextureView;

  if (m_pUnorderedAccessBufferView)
    return m_pUnorderedAccessBufferView;

  return nullptr;
}

XII_ALWAYS_INLINE Diligent::ITextureView* xiiGALUnorderedAccessViewDiligent::GetTextureView()
{
  return m_pUnorderedAccessTextureView;
}

XII_ALWAYS_INLINE Diligent::IBufferView* xiiGALUnorderedAccessViewDiligent::GetBufferView()
{
  return m_pUnorderedAccessBufferView;
}
