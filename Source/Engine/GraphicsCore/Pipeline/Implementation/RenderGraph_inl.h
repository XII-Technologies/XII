#pragma once

// ============================================================================
//  xiiRGPassContext — inline implementations
// ============================================================================

XII_ALWAYS_INLINE xiiGALTexture* xiiRGPassContext::GetTexture(xiiRGTextureHandle handle) const
{
  XII_ASSERT_DEV(handle.IsValid(), "Invalid texture handle passed to GetTexture().");
  XII_ASSERT_DEV(handle.m_uiIndex < static_cast<xiiUInt32>(m_ResolvedTextures.GetCount()),
                 "Texture handle index {} is out of range (max {}).", handle.m_uiIndex, m_ResolvedTextures.GetCount());
  const xiiSharedPtr<xiiGALTexture>& pTexture = m_ResolvedTextures[handle.m_uiIndex];
  XII_ASSERT_DEV(pTexture != nullptr, "Texture at index {} has not been resolved for this frame.", handle.m_uiIndex);
  return pTexture.Borrow();
}

XII_ALWAYS_INLINE xiiGALBuffer* xiiRGPassContext::GetBuffer(xiiRGBufferHandle handle) const
{
  XII_ASSERT_DEV(handle.IsValid(), "Invalid buffer handle passed to GetBuffer().");
  XII_ASSERT_DEV(handle.m_uiIndex < static_cast<xiiUInt32>(m_ResolvedBuffers.GetCount()),
                 "Buffer handle index {} is out of range (max {}).", handle.m_uiIndex, m_ResolvedBuffers.GetCount());
  const xiiSharedPtr<xiiGALBuffer>& pBuffer = m_ResolvedBuffers[handle.m_uiIndex];
  XII_ASSERT_DEV(pBuffer != nullptr, "Buffer at index {} has not been resolved for this frame.", handle.m_uiIndex);
  return pBuffer.Borrow();
}
