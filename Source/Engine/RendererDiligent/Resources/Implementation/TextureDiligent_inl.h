
Diligent::ITexture* xiiGALTextureDiligent::GetTexture()
{
  return m_pExisitingNativeObject == nullptr ? m_pTexture : static_cast<Diligent::ITexture*>(m_pExisitingNativeObject);
}

Diligent::ITexture* xiiGALTextureDiligent::GetStagingTexture()
{
  return m_pStagingTexture;
}

bool xiiGALTextureDiligent::IsNativeWrapperObject()
{
  return m_pExisitingNativeObject != nullptr;
}
