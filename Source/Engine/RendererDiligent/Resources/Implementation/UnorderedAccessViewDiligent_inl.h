
Diligent::IDeviceObject* xiiGALUnorderedAccessViewDiligent::GetResourceView()
{
  if (m_pUnorderedAccessTextureView)
    return m_pUnorderedAccessTextureView;

  if (m_pUnorderedAccessBufferView)
    return m_pUnorderedAccessBufferView;

  return nullptr;
}

Diligent::ITextureView* xiiGALUnorderedAccessViewDiligent::GetTextureView()
{
  return m_pUnorderedAccessTextureView;
}

Diligent::IBufferView* xiiGALUnorderedAccessViewDiligent::GetBufferView()
{
  return m_pUnorderedAccessBufferView;
}
