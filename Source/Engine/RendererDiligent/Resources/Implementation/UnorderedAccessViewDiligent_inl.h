
Diligent::IDeviceObject* xiiGALUnorderedAccessViewDiligent::GetResourceView()
{
  if (m_pUnorderedAccessTextureView)
    return m_pUnorderedAccessTextureView.RawPtr();

  if (m_pUnorderedAccessBufferView)
    return m_pUnorderedAccessBufferView.RawPtr();

  return nullptr;
}

Diligent::ITextureView* xiiGALUnorderedAccessViewDiligent::GetTextureView()
{
  return m_pUnorderedAccessTextureView.RawPtr();
}

Diligent::IBufferView* xiiGALUnorderedAccessViewDiligent::GetBufferView()
{
  return m_pUnorderedAccessBufferView.RawPtr();
}
