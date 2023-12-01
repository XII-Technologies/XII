
XII_ALWAYS_INLINE xiiGALTextureViewHandle xiiGALTextureVulkan::GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType)
{
  /// \todo GraphicsVulkan: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALTextureViewHandle();
}

XII_ALWAYS_INLINE void xiiGALTextureVulkan::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
  Diligent::RESOURCE_STATE requestedStates = xiiDiligentTypeConversions::GetResourceState(stateFlags);

  if (!(m_pTexture->GetState() & requestedStates))
  {
    m_pTexture->SetState(requestedStates);
  }
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALTextureVulkan::GetState() const
{
  return xiiDiligentTypeConversions::GetResourceState(m_pTexture->GetState());
}

XII_ALWAYS_INLINE const xiiGALSparseTextureProperties& xiiGALTextureVulkan::GetSparseProperties() const
{
  /// \todo GraphicsVulkan: Not yet implemented.

  static xiiGALSparseTextureProperties temporary;

  return temporary;
}

XII_ALWAYS_INLINE Diligent::ITexture* xiiGALTextureVulkan::GetTexture() const
{
  return m_pTexture;
}
