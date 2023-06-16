
XII_ALWAYS_INLINE Diligent::ITexture* xiiGALTextureDiligent::GetTexture()
{
  return m_pTexture;
}

XII_ALWAYS_INLINE Diligent::ITexture* xiiGALTextureDiligent::GetStagingTexture()
{
  return m_pStagingTexture;
}

XII_ALWAYS_INLINE bool xiiGALTextureDiligent::IsNativeWrapperObject()
{
  return m_pExisitingNativeObject != nullptr;
}
