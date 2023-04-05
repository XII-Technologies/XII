
Diligent::IDeviceObject* xiiGALResourceViewDiligent::GetResourceView()
{
  if (m_pTextureView)
    return m_pTextureView;

  if (m_pBufferView)
    return m_pBufferView;

  return nullptr;
}

Diligent::ITextureView* xiiGALResourceViewDiligent::GetTextureView()
{
  return m_pTextureView;
}

Diligent::IBufferView* xiiGALResourceViewDiligent::GetBufferView()
{
  return m_pBufferView;
}
