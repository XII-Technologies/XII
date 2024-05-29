
XII_ALWAYS_INLINE void xiiGALBufferVulkan::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALBufferVulkan::GetState() const
{
  return xiiGALResourceStateFlags::Undefined;
}

XII_ALWAYS_INLINE xiiGALMemoryProperties xiiGALBufferVulkan::GetMemoryProperties() const
{
  /// \todo GraphicsVulkan: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALMemoryProperties();
}

XII_ALWAYS_INLINE void xiiGALBufferVulkan::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
}

XII_ALWAYS_INLINE void xiiGALBufferVulkan::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
}

XII_ALWAYS_INLINE xiiGALSparseBufferProperties xiiGALBufferVulkan::GetSparseProperties() const
{
  /// \todo GraphicsVulkan: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALSparseBufferProperties();
}
