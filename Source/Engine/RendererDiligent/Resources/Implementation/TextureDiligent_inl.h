
XII_ALWAYS_INLINE Diligent::ITexture* xiiGALTextureDiligent::GetTexture()
{
  return m_pExisitingNativeObject == nullptr ? m_pTexture : static_cast<Diligent::ITexture*>(m_pExisitingNativeObject);
}

XII_ALWAYS_INLINE Diligent::ITexture* xiiGALTextureDiligent::GetStagingTexture()
{
  return m_pStagingTexture;
}

XII_ALWAYS_INLINE bool xiiGALTextureDiligent::IsNativeWrapperObject()
{
  return m_pExisitingNativeObject != nullptr;
}
