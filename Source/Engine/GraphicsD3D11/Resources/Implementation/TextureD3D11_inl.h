
XII_ALWAYS_INLINE void xiiGALTextureD3D11::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
  Diligent::RESOURCE_STATE requestedStates = xiiDiligentTypeConversions::GetResourceState(stateFlags);

  if (!(m_pTexture->GetState() & requestedStates))
  {
    m_pTexture->SetState(requestedStates);
  }
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALTextureD3D11::GetState() const
{
  return xiiDiligentTypeConversions::GetResourceState(m_pTexture->GetState());
}

XII_ALWAYS_INLINE const xiiGALSparseTextureProperties& xiiGALTextureD3D11::GetSparseProperties() const
{
  /// \todo GraphicsD3D11: Not yet implemented.

  static xiiGALSparseTextureProperties temporary;

  return temporary;
}

XII_ALWAYS_INLINE Diligent::ITexture* xiiGALTextureD3D11::GetTexture() const
{
  return m_pTexture;
}
