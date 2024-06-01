
XII_ALWAYS_INLINE const xiiGALSparseTextureProperties& xiiGALTextureD3D11::GetSparseProperties() const
{
  return m_SparseTextureProperties;
}

XII_ALWAYS_INLINE ID3D11Resource* xiiGALTextureD3D11::GetTexture() const
{
  return m_pTexture;
}
