
#include <Foundation/Memory/FrameAllocator.h>

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
  XII_ASSERT_DEV(hTexture.m_uiIndex < static_cast<xiiUInt32>(m_ResolvedTextures.GetCount()), "Texture handle index {} is out of range (max {}).", hTexture.m_uiIndex, m_ResolvedTextures.GetCount());

  const xiiSharedPtr<xiiGALTexture>& pTexture = m_ResolvedTextures[hTexture.m_uiIndex];
  XII_ASSERT_DEV(pTexture != nullptr, "Texture at index {} has not been resolved for this frame.", hTexture.m_uiIndex);

  return pTexture.Borrow();
}

XII_ALWAYS_INLINE xiiGALBuffer* xiiRGPassContext::GetBuffer(xiiRGBufferHandle hBuffer) const
{
  XII_ASSERT_DEV(hBuffer.IsValid(), "Invalid buffer handle passed to GetBuffer().");
  XII_ASSERT_DEV(hBuffer.m_uiIndex < static_cast<xiiUInt32>(m_ResolvedBuffers.GetCount()), "Buffer handle index {} is out of range (max {}).", hBuffer.m_uiIndex, m_ResolvedBuffers.GetCount());

  const xiiSharedPtr<xiiGALBuffer>& pBuffer = m_ResolvedBuffers[hBuffer.m_uiIndex];
  XII_ASSERT_DEV(pBuffer != nullptr, "Buffer at index {} has not been resolved for this frame.", hBuffer.m_uiIndex);

  return pBuffer.Borrow();
}

template <typename TPassData>
std::pair<TPassData*, xiiRGPassHandle> xiiRenderGraph::AddPass(xiiStringView sName, xiiBitflags<xiiGALCommandQueueFlags> queueFlags, xiiDelegate<void(TPassData&, xiiRGBuilder&)> setupDelegate, xiiDelegate<void(const TPassData&, xiiRGPassContext&)> executeDelegate, bool bHasSideEffects)
{
  XII_ASSERT_DEV(m_bIsSetupOpen, "AddPass must be called between BeginSetup() and EndSetup().");
  XII_ASSERT_DEV(setupDelegate.IsValid(), "Setup function must be valid.");
  XII_ASSERT_DEV(executeDelegate.IsValid(), "Execute function must be valid.");

  const xiiUInt32 uiPassIndex = m_Passes.GetCount();

  PassEntry& passEntry        = m_Passes.ExpandAndGetRef();
  passEntry.m_QueueFlags      = queueFlags;
  passEntry.m_bHasSideEffects = bHasSideEffects;
  passEntry.m_bAllowMerge     = true;
  passEntry.m_sName.Assign(sName);

  TPassData* pData                    = XII_NEW(xiiFrameAllocator::GetCurrentAllocator(), TPassData);
  passEntry.m_pPassData               = pData;
  passEntry.m_DestroyPassDataDelegate = [](void* pData) -> void {
    XII_DELETE(xiiFrameAllocator::GetCurrentAllocator(), static_cast<TPassData*>(pData));
  };

  // Wrap typed execute function in a type-erased delegate.
  passEntry.m_ExecuteDelegate = [executeDelegate, pData](xiiRGPassContext& context) -> void {
    executeDelegate(*pData, context);
  };

  // Call setup delegate immediately, this populates m_Reads / m_Writes via the builder.
  xiiRGBuilder builder(*this, uiPassIndex);
  setupDelegate(*pData, builder);

  m_bIsCompiled = false; // Invalidate any previous compile.

  xiiRGPassHandle hPass;
  hPass.m_uiIndex = uiPassIndex;
  return {pData, hPass};
}

XII_ALWAYS_INLINE const xiiRGStatistics& xiiRenderGraph::GetStatistics() const
{
  return m_Statistics;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRGCompiledPass> xiiRenderGraph::GetCompiledPasses() const
{
  return m_CompiledPasses;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRGBarrierDescription> xiiRenderGraph::GetBarriers() const
{
  return m_Barriers;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRGMergeGroup> xiiRenderGraph::GetMergeGroups() const
{
  return m_MergeGroups;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRGQueueSubmission> xiiRenderGraph::GetQueueSubmissions() const
{
  return m_QueueSubmissions;
}

XII_ALWAYS_INLINE bool xiiRenderGraph::IsCompiled() const
{
  return m_bIsCompiled;
}
