
XII_ALWAYS_INLINE void xiiGALBufferVulkan::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
  Diligent::RESOURCE_STATE requestedStates = xiiDiligentTypeConversions::GetResourceState(stateFlags);

  if (!(m_pBuffer->GetState() & requestedStates))
  {
    m_pBuffer->SetState(requestedStates);
  }
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALBufferVulkan::GetState() const
{
  return xiiDiligentTypeConversions::GetResourceState(m_pBuffer->GetState());
}

XII_ALWAYS_INLINE xiiGALMemoryProperties xiiGALBufferVulkan::GetMemoryProperties() const
{
  /// \todo GraphicsVulkan: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALMemoryProperties();
}

XII_ALWAYS_INLINE void xiiGALBufferVulkan::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  m_pBuffer->FlushMappedRange(uiStartOffset, uiSize);
}

XII_ALWAYS_INLINE void xiiGALBufferVulkan::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  m_pBuffer->InvalidateMappedRange(uiStartOffset, uiSize);
}

XII_ALWAYS_INLINE xiiGALSparseBufferProperties xiiGALBufferVulkan::GetSparseProperties() const
{
  /// \todo GraphicsVulkan: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALSparseBufferProperties();
}

XII_ALWAYS_INLINE Diligent::IBuffer* xiiGALBufferVulkan::GetBuffer() const
{
  return m_pBuffer;
}

XII_ALWAYS_INLINE Diligent::VALUE_TYPE xiiGALBufferVulkan::GetIndexFormat() const
{
  return m_IndexFormat;
}
