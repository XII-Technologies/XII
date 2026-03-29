
XII_ALWAYS_INLINE xiiGALCommandList& xiiRGPassContext::GetCommandList() const
{
  XII_ASSERT_DEV(m_pCommandList != nullptr, "Command list is null.");

  return *m_pCommandList;
}

XII_ALWAYS_INLINE xiiRenderGraphBlackboard& xiiRGPassContext::GetBlackboard() const
{
  XII_ASSERT_DEV(m_pBlackboard != nullptr, "Blackboard is null.");

  return *m_pBlackboard;
}

XII_ALWAYS_INLINE xiiRenderGraphResourceCache& xiiRGPassContext::GetResourceCache() const
{
  XII_ASSERT_DEV(m_pResourceCache != nullptr, "ResourceCache is null.");

  return *m_pResourceCache;
}

XII_ALWAYS_INLINE const xiiView* xiiRGPassContext::GetView() const
{
  return m_pView;
}

XII_ALWAYS_INLINE xiiUInt64 xiiRGPassContext::GetFrameIndex() const
{
  return m_uiFrameIndex;
}

XII_ALWAYS_INLINE xiiHashedString xiiRGPassContext::GetPassName() const
{
  return m_sPassName;
}


XII_ALWAYS_INLINE xiiGALTexture* xiiRGPassContext::GetTexture(xiiRGTextureHandle hTexture) const
{
  XII_ASSERT_DEV(hTexture.IsValid(), "Invalid texture handle passed to GetTexture().");
  XII_ASSERT_DEV(hTexture.m_uiIndex < static_cast<xiiUInt32>(m_ResolvedTextures.GetCount()),
                 "Texture handle index {} is out of range (max {}).", hTexture.m_uiIndex, m_ResolvedTextures.GetCount());
  const xiiSharedPtr<xiiGALTexture>& pTexture = m_ResolvedTextures[hTexture.m_uiIndex];
  XII_ASSERT_DEV(pTexture != nullptr, "Texture at index {} has not been resolved for this frame.", hTexture.m_uiIndex);
  return pTexture.Borrow();
}

XII_ALWAYS_INLINE xiiGALBuffer* xiiRGPassContext::GetBuffer(xiiRGBufferHandle hBuffer) const
{
  XII_ASSERT_DEV(hBuffer.IsValid(), "Invalid buffer handle passed to GetBuffer().");
  XII_ASSERT_DEV(hBuffer.m_uiIndex < static_cast<xiiUInt32>(m_ResolvedBuffers.GetCount()),
                 "Buffer handle index {} is out of range (max {}).", hBuffer.m_uiIndex, m_ResolvedBuffers.GetCount());
  const xiiSharedPtr<xiiGALBuffer>& pBuffer = m_ResolvedBuffers[hBuffer.m_uiIndex];
  XII_ASSERT_DEV(pBuffer != nullptr, "Buffer at index {} has not been resolved for this frame.", hBuffer.m_uiIndex);
  return pBuffer.Borrow();
}
