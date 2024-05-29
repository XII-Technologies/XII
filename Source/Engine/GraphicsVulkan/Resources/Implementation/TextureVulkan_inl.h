
XII_ALWAYS_INLINE void xiiGALTextureVulkan::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALTextureVulkan::GetState() const
{
  return xiiGALResourceStateFlags::Undefined;
}

XII_ALWAYS_INLINE const xiiGALSparseTextureProperties& xiiGALTextureVulkan::GetSparseProperties() const
{
  /// \todo GraphicsVulkan: Not yet implemented.

  static xiiGALSparseTextureProperties temporary;

  return temporary;
}
