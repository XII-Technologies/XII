/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/Memory/FrameAllocator.h>

XII_ALWAYS_INLINE xiiGALCommandList& xiiRenderGraphPassContext::GetCommandList() const
{
  XII_ASSERT_DEV(m_pCommandList != nullptr, "Command list is null.");

  return *m_pCommandList;
}

XII_ALWAYS_INLINE const xiiSharedPtr<xiiGALRenderPass>& xiiRenderGraphPassContext::GetRenderPass() const
{
  return m_pRenderPass;
}

XII_ALWAYS_INLINE xiiUInt32 xiiRenderGraphPassContext::GetSubpassIndex() const
{
  return m_uiSubpassIndex;
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
  XII_ASSERT_DEV(m_pGraph != nullptr && m_pGraph->GetTextureDescription(hTexture) != nullptr, "Texture handle belongs to another graph setup or resource domain.");
  if (m_pGraph == nullptr || m_pGraph->GetTextureDescription(hTexture) == nullptr)
    return nullptr;
  const xiiUInt32 uiResourceIndex = hTexture.GetInternalID().m_InstanceIndex;
  XII_ASSERT_DEV(uiResourceIndex < static_cast<xiiUInt32>(m_ResolvedTextures.GetCount()), "Texture handle index {} is out of range (max {}).", uiResourceIndex, m_ResolvedTextures.GetCount());

  const xiiSharedPtr<xiiGALTexture>& pTexture = m_ResolvedTextures[uiResourceIndex];
  XII_ASSERT_DEV(pTexture != nullptr, "Texture at index {} has not been resolved for this frame.", uiResourceIndex);

  return pTexture.Borrow();
}

XII_ALWAYS_INLINE xiiGALBuffer* xiiRenderGraphPassContext::GetBuffer(xiiRenderGraphBufferHandle hBuffer) const
{
  XII_ASSERT_DEV(m_pGraph != nullptr && m_pGraph->GetBufferDescription(hBuffer) != nullptr, "Buffer handle belongs to another graph setup or resource domain.");
  if (m_pGraph == nullptr || m_pGraph->GetBufferDescription(hBuffer) == nullptr)
    return nullptr;
  const xiiUInt32 uiResourceIndex = hBuffer.GetInternalID().m_InstanceIndex;
  XII_ASSERT_DEV(uiResourceIndex < static_cast<xiiUInt32>(m_ResolvedBuffers.GetCount()), "Buffer handle index {} is out of range (max {}).", uiResourceIndex, m_ResolvedBuffers.GetCount());

  const xiiSharedPtr<xiiGALBuffer>& pBuffer = m_ResolvedBuffers[uiResourceIndex];
  XII_ASSERT_DEV(pBuffer != nullptr, "Buffer at index {} has not been resolved for this frame.", uiResourceIndex);

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
  passEntry.m_Id              = xiiRenderGraphId(uiPassIndex, m_uiSetupSerial);
  passEntry.m_QueueFlags      = queueFlags;
  passEntry.m_bHasSideEffects = bHasSideEffects;
  passEntry.m_bAllowMerge     = true;
  passEntry.m_sName.Assign(sName);
  passEntry.m_pBlackboard = m_pSetupBlackboard;

  TPassData* pPassData                = XII_NEW(xiiAlignedAllocatorWrapper::GetAllocator(), TPassData);
  passEntry.m_pPassData               = pPassData;
  passEntry.m_DestroyPassDataDelegate = [](void* pData) -> void {
    xiiInternal::Delete(xiiAlignedAllocatorWrapper::GetAllocator(), static_cast<TPassData*>(pData));
  };

  // Wrap typed execute function in a type-erased delegate.
  passEntry.m_ExecuteDelegate = [executeDelegate, pPassData](xiiRenderGraphPassContext& context) -> void {
    executeDelegate(*pPassData, context);
  };

  // Call setup delegate immediately, this populates m_Reads / m_Writes via the builder.
  xiiRenderGraphBuilder builder(*this, uiPassIndex);
  setupDelegate(*pPassData, builder);

  m_bIsCompiled = false; // Invalidate any previous compile.

  const xiiRenderGraphPassHandle hPass(passEntry.m_Id);
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

XII_ALWAYS_INLINE xiiRenderGraphId xiiRenderGraph::GetId() const
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

XII_ALWAYS_INLINE xiiSharedPtr<xiiGALTexture> xiiRenderGraph::GetExportedTexture(xiiRenderGraphTextureHandle hTexture) const
{
  if (GetTextureDescription(hTexture) == nullptr)
    return {};
  xiiSharedPtr<xiiGALTexture> pTexture;
  m_ExportedTextures.TryGetValue(hTexture.GetResourceId().m_Data, pTexture);
  return pTexture;
}

XII_ALWAYS_INLINE xiiSharedPtr<xiiGALBuffer> xiiRenderGraph::GetExportedBuffer(xiiRenderGraphBufferHandle hBuffer) const
{
  if (GetBufferDescription(hBuffer) == nullptr)
    return {};
  xiiSharedPtr<xiiGALBuffer> pBuffer;
  m_ExportedBuffers.TryGetValue(hBuffer.GetResourceId().m_Data, pBuffer);
  return pBuffer;
}
