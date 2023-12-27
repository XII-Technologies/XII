
XII_ALWAYS_INLINE xiiGALTextureViewHandle xiiGALTexture::GetDefaultView(xiiEnum<xiiGALTextureViewType> viewType)
{
  switch (viewType)
  {
    case xiiGALTextureViewType::ShaderResource:
      return m_hDefaultTextureView;

    case xiiGALTextureViewType::RenderTarget:
    case xiiGALTextureViewType::DepthStencil:
      return m_hDefaultRenderTargetView;

    default:
      return xiiGALTextureViewHandle();
  }
}

XII_FORCE_INLINE bool xiiGALTextureCreationDescription::IsArray() const
{
  return m_Type == xiiGALResourceDimension::Texture1DArray || m_Type == xiiGALResourceDimension::Texture2DArray || m_Type == xiiGALResourceDimension::TextureCube || m_Type == xiiGALResourceDimension::TextureCubeArray;
}

XII_FORCE_INLINE bool xiiGALTextureCreationDescription::Is1D() const
{
  return m_Type == xiiGALResourceDimension::Texture1D || m_Type == xiiGALResourceDimension::Texture1DArray;
}

XII_FORCE_INLINE bool xiiGALTextureCreationDescription::Is2D() const
{
  return m_Type == xiiGALResourceDimension::Texture2D || m_Type == xiiGALResourceDimension::Texture2DArray || m_Type == xiiGALResourceDimension::TextureCube || m_Type == xiiGALResourceDimension::TextureCubeArray;
}

XII_FORCE_INLINE bool xiiGALTextureCreationDescription::Is3D() const
{
  return m_Type == xiiGALResourceDimension::Texture3D;
}

XII_FORCE_INLINE bool xiiGALTextureCreationDescription::IsCube() const
{
  return m_Type == xiiGALResourceDimension::TextureCube || m_Type == xiiGALResourceDimension::TextureCubeArray;
};
