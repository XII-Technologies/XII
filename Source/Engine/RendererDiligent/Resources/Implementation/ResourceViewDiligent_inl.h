
XII_ALWAYS_INLINE Diligent::IDeviceObject* xiiGALResourceViewDiligent::GetResourceView()
{
  if (m_pTextureView)
    return m_pTextureView;

  if (m_pBufferView)
    return m_pBufferView;

  return nullptr;
}

XII_ALWAYS_INLINE Diligent::ITextureView* xiiGALResourceViewDiligent::GetTextureView()
{
  return m_pTextureView;
}

XII_ALWAYS_INLINE Diligent::IBufferView* xiiGALResourceViewDiligent::GetBufferView()
{
  return m_pBufferView;
}
