
XII_ALWAYS_INLINE void xiiGALBufferD3D12::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALBufferD3D12::GetState() const
{
  return {};
}

XII_ALWAYS_INLINE xiiGALMemoryProperties xiiGALBufferD3D12::GetMemoryProperties() const
{
  /// \todo GraphicsD3D12: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALMemoryProperties();
}

XII_ALWAYS_INLINE void xiiGALBufferD3D12::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
}

XII_ALWAYS_INLINE void xiiGALBufferD3D12::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
}

XII_ALWAYS_INLINE xiiGALSparseBufferProperties xiiGALBufferD3D12::GetSparseProperties() const
{
  /// \todo GraphicsD3D12: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALSparseBufferProperties();
}
