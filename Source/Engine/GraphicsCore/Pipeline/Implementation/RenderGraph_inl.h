/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Memory/FrameAllocator.h>

XII_ALWAYS_INLINE xiiGALCommandList& xiiRenderGraphPassContext::GetCommandList() const
{
  XII_ASSERT_DEV(m_pCommandList != nullptr, "Command list is null.");

  return *m_pCommandList;
}

XII_ALWAYS_INLINE xiiRenderGraphBlackboard& xiiRenderGraphPassContext::GetBlackboard() const
{
  XII_ASSERT_DEV(m_pBlackboard != nullptr, "Blackboard is null.");

  return *m_pBlackboard;
}

XII_ALWAYS_INLINE xiiRenderGraphResourceCache& xiiRenderGraphPassContext::GetResourceCache() const
{
  XII_ASSERT_DEV(m_pResourceCache != nullptr, "ResourceCache is null.");

  return *m_pResourceCache;
}

XII_ALWAYS_INLINE const xiiView* xiiRenderGraphPassContext::GetView() const
{
  return m_pView;
}

XII_ALWAYS_INLINE xiiUInt64 xiiRenderGraphPassContext::GetFrameIndex() const
{
  return m_uiFrameIndex;
}

XII_ALWAYS_INLINE xiiHashedString xiiRenderGraphPassContext::GetPassName() const
{
  return m_sPassName;
}

XII_ALWAYS_INLINE xiiGALTexture* xiiRenderGraphPassContext::GetTexture(xiiRenderGraphTextureHandle hTexture) const
{
  XII_ASSERT_DEV(hTexture.IsValid(), "Invalid texture handle passed to GetTexture().");
  XII_ASSERT_DEV(hTexture.m_uiIndex < static_cast<xiiUInt32>(m_ResolvedTextures.GetCount()), "Texture handle index {} is out of range (max {}).", hTexture.m_uiIndex, m_ResolvedTextures.GetCount());

  const xiiSharedPtr<xiiGALTexture>& pTexture = m_ResolvedTextures[hTexture.m_uiIndex];
  XII_ASSERT_DEV(pTexture != nullptr, "Texture at index {} has not been resolved for this frame.", hTexture.m_uiIndex);

  return pTexture.Borrow();
}

XII_ALWAYS_INLINE xiiGALBuffer* xiiRenderGraphPassContext::GetBuffer(xiiRenderGraphBufferHandle hBuffer) const
{
  XII_ASSERT_DEV(hBuffer.IsValid(), "Invalid buffer handle passed to GetBuffer().");
  XII_ASSERT_DEV(hBuffer.m_uiIndex < static_cast<xiiUInt32>(m_ResolvedBuffers.GetCount()), "Buffer handle index {} is out of range (max {}).", hBuffer.m_uiIndex, m_ResolvedBuffers.GetCount());

  const xiiSharedPtr<xiiGALBuffer>& pBuffer = m_ResolvedBuffers[hBuffer.m_uiIndex];
  XII_ASSERT_DEV(pBuffer != nullptr, "Buffer at index {} has not been resolved for this frame.", hBuffer.m_uiIndex);

  return pBuffer.Borrow();
}

template <typename TPassData>
std::pair<TPassData*, xiiRenderGraphPassHandle> xiiRenderGraph::AddPass(xiiStringView sName, xiiBitflags<xiiGALCommandQueueFlags> queueFlags, xiiDelegate<void(TPassData&, xiiRenderGraphBuilder&)> setupDelegate, xiiDelegate<void(const TPassData&, xiiRenderGraphPassContext&)> executeDelegate, bool bHasSideEffects)
{
  XII_ASSERT_DEV(m_bIsSetupOpen, "AddPass must be called between BeginSetup() and EndSetup().");
  XII_ASSERT_DEV(setupDelegate.IsValid(), "Setup function must be valid.");
  XII_ASSERT_DEV(executeDelegate.IsValid(), "Execute function must be valid.");

  const xiiUInt32 uiPassIndex = m_Passes.GetCount();

  PassEntry& passEntry        = m_Passes.ExpandAndGetRef();
  passEntry.m_Id.m_uiValue    = xiiHashingUtils::xxHash64String(sName, m_Id.m_uiValue);
  passEntry.m_Id.m_uiValue    = xiiHashingUtils::xxHash64(&uiPassIndex, sizeof(uiPassIndex), passEntry.m_Id.m_uiValue);
  passEntry.m_QueueFlags      = queueFlags;
  passEntry.m_bHasSideEffects = bHasSideEffects;
  passEntry.m_bAllowMerge     = true;
  passEntry.m_sName.Assign(sName);

  TPassData* pPassData                = XII_NEW(xiiDefaultAllocatorWrapper::GetAllocator(), TPassData);
  passEntry.m_pPassData               = pPassData;
  passEntry.m_DestroyPassDataDelegate = [](void* pData) -> void {
    xiiInternal::Delete(xiiDefaultAllocatorWrapper::GetAllocator(), static_cast<TPassData*>(pData));
  };

  // Wrap typed execute function in a type-erased delegate.
  passEntry.m_ExecuteDelegate = [executeDelegate, pPassData](xiiRenderGraphPassContext& context) -> void {
    executeDelegate(*pPassData, context);
  };

  // Call setup delegate immediately, this populates m_Reads / m_Writes via the builder.
  xiiRenderGraphBuilder builder(*this, uiPassIndex);
  setupDelegate(*pPassData, builder);

  m_bIsCompiled = false; // Invalidate any previous compile.

  xiiRenderGraphPassHandle hPass;
  hPass.m_uiIndex = uiPassIndex;
  hPass.m_Id      = passEntry.m_Id;
  return {pPassData, hPass};
}

XII_ALWAYS_INLINE const xiiRenderGraphStatistics& xiiRenderGraph::GetStatistics() const
{
  return m_Statistics;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRenderGraphCompiledPass> xiiRenderGraph::GetCompiledPasses() const
{
  return m_CompiledPasses;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRenderGraphBarrierDescription> xiiRenderGraph::GetBarriers() const
{
  return m_Barriers;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRenderGraphMergeGroup> xiiRenderGraph::GetMergeGroups() const
{
  return m_MergeGroups;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRenderGraphQueueSubmission> xiiRenderGraph::GetQueueSubmissions() const
{
  return m_QueueSubmissions;
}

XII_ALWAYS_INLINE bool xiiRenderGraph::IsCompiled() const
{
  return m_bIsCompiled;
}

XII_ALWAYS_INLINE xiiRenderGraphGraphId xiiRenderGraph::GetId() const
{
  return m_Id;
}

XII_ALWAYS_INLINE xiiStringView xiiRenderGraph::GetName() const
{
  return m_sName.GetView();
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRenderGraphResourceDescription> xiiRenderGraph::GetResourceDescriptions() const
{
  return m_ResourceDescriptions;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiRenderGraphResourceVersionDescription> xiiRenderGraph::GetResourceVersions() const
{
  return m_ResourceVersions;
}

XII_ALWAYS_INLINE const xiiRenderGraphIdTable& xiiRenderGraph::GetIdTable() const
{
  return m_IdTable;
}

XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> xiiRenderGraph::GetExportedTexture(xiiRenderGraphTextureHandle hTexture) const
{
  xiiSharedPtr<xiiGALTexture> pTexture;
  m_ExportedTextures.TryGetValue(hTexture.m_Id.m_uiValue, pTexture);
  return pTexture;
}

XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> xiiRenderGraph::GetExportedBuffer(xiiRenderGraphBufferHandle hBuffer) const
{
  xiiSharedPtr<xiiGALBuffer> pBuffer;
  m_ExportedBuffers.TryGetValue(hBuffer.m_Id.m_uiValue, pBuffer);
  return pBuffer;
}
