
XII_ALWAYS_INLINE Diligent::IDeviceObject* xiiGALUnorderedAccessViewDiligent::GetResourceView()
{
  if (m_pTextureView)
    return m_pTextureView;

  if (m_pBufferView)
    return m_pBufferView;

  return nullptr;
}

XII_ALWAYS_INLINE Diligent::ITextureView* xiiGALUnorderedAccessViewDiligent::GetTextureView()
{
  return m_pTextureView;
}

XII_ALWAYS_INLINE Diligent::IBufferView* xiiGALUnorderedAccessViewDiligent::GetBufferView()
{
  return m_pBufferView;
}
