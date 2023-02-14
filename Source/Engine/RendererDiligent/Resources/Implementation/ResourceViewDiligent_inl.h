
Diligent::IDeviceObject* xiiGALResourceViewDiligent::GetResourceView()
{
  if (m_pTextureView)
    return m_pTextureView.RawPtr();

  if (m_pBufferView)
    return m_pBufferView.RawPtr();

  return nullptr;
}

Diligent::ITextureView* xiiGALResourceViewDiligent::GetTextureView()
{
  return m_pTextureView.RawPtr();
}

Diligent::IBufferView* xiiGALResourceViewDiligent::GetBufferView()
{
  return m_pBufferView.RawPtr();
}
