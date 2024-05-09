
XII_ALWAYS_INLINE void xiiGALTextureD3D12::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALTextureD3D12::GetState() const
{
  return {};
}

XII_ALWAYS_INLINE const xiiGALSparseTextureProperties& xiiGALTextureD3D12::GetSparseProperties() const
{
  /// \todo GraphicsD3D12: Not yet implemented.

  static xiiGALSparseTextureProperties temporary;

  return temporary;
}
