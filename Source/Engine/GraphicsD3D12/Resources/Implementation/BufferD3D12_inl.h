
XII_ALWAYS_INLINE xiiGALBufferViewHandle xiiGALBufferD3D12::GetDefaultView(xiiEnum<xiiGALBufferViewType> viewType)
{
  /// \todo GraphicsD3D12: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALBufferViewHandle();
}

XII_ALWAYS_INLINE void xiiGALBufferD3D12::SetState(xiiBitflags<xiiGALResourceStateFlags> stateFlags)
{
  Diligent::RESOURCE_STATE requestedStates = xiiDiligentTypeConversions::GetResourceState(stateFlags);

  if (!(m_pBuffer->GetState() & requestedStates))
  {
    m_pBuffer->SetState(requestedStates);
  }
}

XII_ALWAYS_INLINE xiiBitflags<xiiGALResourceStateFlags> xiiGALBufferD3D12::GetState() const
{
  return xiiDiligentTypeConversions::GetResourceState(m_pBuffer->GetState());
}

XII_ALWAYS_INLINE xiiGALMemoryProperties xiiGALBufferD3D12::GetMemoryProperties() const
{
  /// \todo GraphicsD3D12: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALMemoryProperties();
}

XII_ALWAYS_INLINE void xiiGALBufferD3D12::FlushMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  m_pBuffer->FlushMappedRange(uiStartOffset, uiSize);
}

XII_ALWAYS_INLINE void xiiGALBufferD3D12::InvalidateMappedRange(xiiUInt64 uiStartOffset, xiiUInt64 uiSize)
{
  m_pBuffer->InvalidateMappedRange(uiStartOffset, uiSize);
}

XII_ALWAYS_INLINE xiiGALSparseBufferProperties xiiGALBufferD3D12::GetSparseProperties() const
{
  /// \todo GraphicsD3D12: Not yet implemented.

  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiGALSparseBufferProperties();
}

XII_ALWAYS_INLINE Diligent::IBuffer* xiiGALBufferD3D12::GetBuffer() const
{
  return m_pBuffer;
}
