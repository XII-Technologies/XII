#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/Deque.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderGraphDebug.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/CommandEncoder/CommandQueue.h>
#include <GraphicsFoundation/Declarations/Descriptors.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Tools/ScopedDebugGroup.h>

xiiRGBuilder::xiiRGBuilder(xiiRenderGraph& graph, xiiUInt32 uiPassIndex) :
  m_Graph(graph), m_uiPassIndex(uiPassIndex)
{
}

xiiRGTextureHandle xiiRGBuilder::DeclareTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description)
{
  xiiUInt32 uiTextureResourceIndex = xiiInvalidIndex;
  if (m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiTextureResourceIndex))
  {
    XII_ASSERT_DEV(m_Graph.m_Resources[uiTextureResourceIndex].m_bIsTexture, "Resource '{}' was already declared as a buffer.", sName);

    xiiRGTextureHandle hTexture;
    hTexture.m_uiIndex   = uiTextureResourceIndex;
    hTexture.m_uiVersion = m_Graph.m_Resources[uiTextureResourceIndex].m_uiCurrentVersion;
    return hTexture;
  }

  uiTextureResourceIndex               = m_Graph.m_Resources.GetCount();
  xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
  entry.m_bIsTexture                   = true;
  entry.m_bIsImported                  = false;
  entry.m_bIsTransient                 = true;
  entry.m_TextureDescription           = description;

  entry.m_sName.Assign(sName);
  m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiTextureResourceIndex);

  xiiRGTextureHandle hTexture;
  hTexture.m_uiIndex   = uiTextureResourceIndex;
  hTexture.m_uiVersion = 0U;
  return hTexture;
}

xiiRGTextureHandle xiiRGBuilder::ImportTexture(xiiStringView sName, xiiSharedPtr<xiiGALTexture> pTexture, xiiBitflags<xiiGALResourceStateFlags> currentState)
{
  XII_ASSERT_DEV(pTexture != nullptr, "Cannot import a null texture.");

  xiiUInt32 uiTextureResourceIndex = xiiInvalidIndex;
  if (!m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiTextureResourceIndex))
  {
    uiTextureResourceIndex               = m_Graph.m_Resources.GetCount();
    xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
    entry.m_bIsTexture                   = true;
    entry.m_bIsImported                  = true;
    entry.m_bIsTransient                 = false;
    entry.m_pImportedTexture             = pTexture;
    entry.m_ImportedInitialState         = currentState;
    entry.m_CurrentState                 = currentState;

    entry.m_sName.Assign(sName);
    m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiTextureResourceIndex);
  }

  xiiRGTextureHandle hTexture;
  hTexture.m_uiIndex   = uiTextureResourceIndex;
  hTexture.m_uiVersion = m_Graph.m_Resources[uiTextureResourceIndex].m_uiCurrentVersion;
  return hTexture;
}

xiiRGTextureHandle xiiRGBuilder::ReadTexture(xiiRGTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(hTexture.IsValid(), "Invalid texture handle.");
  XII_ASSERT_DEV(hTexture.m_uiIndex < m_Graph.m_Resources.GetCount(), "Handle index out of range.");

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = hTexture.m_uiIndex;
  resourceUsage.m_bIsTexture      = true;
  resourceUsage.m_uiVersion       = hTexture.m_uiVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = false;
  m_Graph.m_Passes[m_uiPassIndex].m_Reads.PushBack(resourceUsage);

  return hTexture;
}

xiiRGTextureHandle xiiRGBuilder::WriteTexture(xiiRGTextureHandle hTexture, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(hTexture.IsValid(), "Invalid texture handle.");
  XII_ASSERT_DEV(hTexture.m_uiIndex < m_Graph.m_Resources.GetCount(), "Handle index out of range.");

  xiiRenderGraph::ResourceEntry& resourceEntry = m_Graph.m_Resources[hTexture.m_uiIndex];
  resourceEntry.m_uiCurrentProducerPassIdx     = m_uiPassIndex;
  ++resourceEntry.m_uiCurrentVersion;

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = hTexture.m_uiIndex;
  resourceUsage.m_bIsTexture      = true;
  resourceUsage.m_uiVersion       = resourceEntry.m_uiCurrentVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = true;
  m_Graph.m_Passes[m_uiPassIndex].m_Writes.PushBack(resourceUsage);

  xiiRGTextureHandle hNewTexture;
  hNewTexture.m_uiIndex   = hTexture.m_uiIndex;
  hNewTexture.m_uiVersion = resourceEntry.m_uiCurrentVersion;
  return hNewTexture;
}

xiiRGTextureHandle xiiRGBuilder::WriteTexture(xiiStringView sName, const xiiGALTextureCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  return WriteTexture(DeclareTexture(sName, description), requiredState);
}

xiiRGBufferHandle xiiRGBuilder::DeclareBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description)
{
  xiiUInt32 uiBufferResourceIndex = xiiInvalidIndex;
  if (m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiBufferResourceIndex))
  {
    XII_ASSERT_DEV(!m_Graph.m_Resources[uiBufferResourceIndex].m_bIsTexture, "Resource '{}' was already declared as a texture.", sName);

    xiiRGBufferHandle hBuffer;
    hBuffer.m_uiIndex   = uiBufferResourceIndex;
    hBuffer.m_uiVersion = m_Graph.m_Resources[uiBufferResourceIndex].m_uiCurrentVersion;
    return hBuffer;
  }

  uiBufferResourceIndex                = m_Graph.m_Resources.GetCount();
  xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
  entry.m_bIsTexture                   = false;
  entry.m_bIsImported                  = false;
  entry.m_bIsTransient                 = true;
  entry.m_BufferDescription            = description;

  entry.m_sName.Assign(sName);
  m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiBufferResourceIndex);

  xiiRGBufferHandle hBuffer;
  hBuffer.m_uiIndex   = uiBufferResourceIndex;
  hBuffer.m_uiVersion = 0U;
  return hBuffer;
}

xiiRGBufferHandle xiiRGBuilder::ImportBuffer(xiiStringView sName, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiBitflags<xiiGALResourceStateFlags> currentState)
{
  XII_ASSERT_DEV(pBuffer != nullptr, "Cannot import a null buffer.");

  xiiUInt32 uiBufferResourceIndex = xiiInvalidIndex;
  if (!m_Graph.m_ResourceNameIndex.TryGetValue(xiiTempHashedString(sName), uiBufferResourceIndex))
  {
    uiBufferResourceIndex                = m_Graph.m_Resources.GetCount();
    xiiRenderGraph::ResourceEntry& entry = m_Graph.m_Resources.ExpandAndGetRef();
    entry.m_bIsTexture                   = false;
    entry.m_bIsImported                  = true;
    entry.m_bIsTransient                 = false;
    entry.m_pImportedBuffer              = pBuffer;
    entry.m_ImportedInitialState         = currentState;
    entry.m_CurrentState                 = currentState;

    entry.m_sName.Assign(sName);
    m_Graph.m_ResourceNameIndex.Insert(entry.m_sName, uiBufferResourceIndex);
  }

  xiiRGBufferHandle hBuffer;
  hBuffer.m_uiIndex   = uiBufferResourceIndex;
  hBuffer.m_uiVersion = m_Graph.m_Resources[uiBufferResourceIndex].m_uiCurrentVersion;
  return hBuffer;
}

xiiRGBufferHandle xiiRGBuilder::ReadBuffer(xiiRGBufferHandle hBuffer, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(hBuffer.IsValid(), "Invalid buffer handle.");

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = hBuffer.m_uiIndex;
  resourceUsage.m_bIsTexture      = false;
  resourceUsage.m_uiVersion       = hBuffer.m_uiVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = false;
  m_Graph.m_Passes[m_uiPassIndex].m_Reads.PushBack(resourceUsage);

  return hBuffer;
}

xiiRGBufferHandle xiiRGBuilder::WriteBuffer(xiiRGBufferHandle handle, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  XII_ASSERT_DEV(handle.IsValid(), "Invalid buffer handle.");

  xiiRenderGraph::ResourceEntry& resourceEntry = m_Graph.m_Resources[handle.m_uiIndex];
  resourceEntry.m_uiCurrentProducerPassIdx     = m_uiPassIndex;
  ++resourceEntry.m_uiCurrentVersion;

  xiiRenderGraph::ResourceUsage resourceUsage;
  resourceUsage.m_uiResourceIndex = handle.m_uiIndex;
  resourceUsage.m_bIsTexture      = false;
  resourceUsage.m_uiVersion       = resourceEntry.m_uiCurrentVersion;
  resourceUsage.m_RequiredState   = requiredState;
  resourceUsage.m_bIsWrite        = true;
  m_Graph.m_Passes[m_uiPassIndex].m_Writes.PushBack(resourceUsage);

  xiiRGBufferHandle hNewBuffer;
  hNewBuffer.m_uiIndex   = handle.m_uiIndex;
  hNewBuffer.m_uiVersion = resourceEntry.m_uiCurrentVersion;
  return hNewBuffer;
}

xiiRGBufferHandle xiiRGBuilder::WriteBuffer(xiiStringView sName, const xiiGALBufferCreationDescription& description, xiiBitflags<xiiGALResourceStateFlags> requiredState)
{
  return WriteBuffer(DeclareBuffer(sName, description), requiredState);
}

void xiiRGBuilder::SetPassSideEffects(bool bHasSideEffects)
{
  m_Graph.m_Passes[m_uiPassIndex].m_bHasSideEffects = bHasSideEffects;
}

void xiiRGBuilder::SetPassAllowMerge(bool bAllowMerge)
{
  m_Graph.m_Passes[m_uiPassIndex].m_bAllowMerge = bAllowMerge;
}

//////////////////////////////////////////////////////////////////////////

xiiRenderGraph::xiiRenderGraph() = default;

xiiRenderGraph::~xiiRenderGraph()
{
  // Destroy heap-allocated pass data.
  for (PassEntry& entry : m_Passes)
  {
    if (entry.m_pPassData && entry.m_DestroyPassDataDelegate.IsValid())
    {
      entry.m_DestroyPassDataDelegate(entry.m_pPassData);
      entry.m_pPassData = nullptr;
    }
  }
}

void xiiRenderGraph::BeginSetup(xiiUInt64 uiFrameIndex)
{
  XII_ASSERT_DEV(!m_bIsSetupOpen, "BeginSetup called while setup was already open.");

  // Destroy pass data from previous frame.
  for (PassEntry& entry : m_Passes)
  {
    if (entry.m_pPassData && entry.m_DestroyPassDataDelegate.IsValid())
    {
      entry.m_DestroyPassDataDelegate(entry.m_pPassData);
    }
  }

  m_Passes.Clear();
  m_Resources.Clear();
  m_ResourceNameIndex.Clear();

  m_uiFrameIndex = uiFrameIndex;
  m_bIsSetupOpen = true;
  m_bIsCompiled  = false;
}

void xiiRenderGraph::EndSetup()
{
  XII_ASSERT_DEV(m_bIsSetupOpen, "EndSetup called without a matching BeginSetup.");

  m_bIsSetupOpen = false;
}

xiiResult xiiRenderGraph::Compile(const xiiRGCompileSettings& settings, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(!m_bIsSetupOpen, "Cannot compile while setup is still open.");

  m_CompiledPasses.Clear();
  m_Barriers.Clear();
  m_MergeGroups.Clear();
  m_QueueSubmissions.Clear();
  m_Statistics                         = {};
  m_Statistics.m_uiRegisteredPassCount = m_Passes.GetCount();

  // Phase G pre-check: compute signature, if unchanged and cache enabled -> skip recompile.
  PhaseG_SignatureAndCache(settings);
  if (m_Statistics.m_bUsedCachedCompile)
    return XII_SUCCESS;

  // Phase B: topological sort + culling.
  xiiDynamicArray<xiiUInt32> sortedIndices;
  PhaseB_TopologicalSortAndCull(settings, sortedIndices);

  if (sortedIndices.IsEmpty() && m_Passes.IsEmpty())
  {
    m_bIsCompiled = true;
    return XII_SUCCESS;
  }

  // Phase C: transient resource lifetime analysis.
  PhaseC_LifetimeAnalysis(sortedIndices);

  // Phase D: barrier synthesis.
  PhaseD_BarrierSynthesis(sortedIndices, settings);

  // Phase E: multi-queue scheduling.
  // Pass nullptr device here, queue checks happen at Execute time.
  PhaseE_MultiQueueScheduling(sortedIndices, nullptr, settings);

  // Phase F: render-pass merging (device needed to create native render passes, deferred to Execute for the first frame, then cached).
  // Merging is completed during Execute once a device is available.

  m_bIsCompiled                         = true;
  m_Statistics.m_uiCompiledPassCount    = m_CompiledPasses.GetCount();
  m_Statistics.m_uiTotalBarrierCount    = m_Barriers.GetCount();
  m_Statistics.m_uiQueueSubmissionCount = m_QueueSubmissions.GetCount();

  for (const xiiRGBarrierDescription& barrier : m_Barriers)
  {
    if (barrier.m_TransitionType != xiiGALStateTransitionType::Immediate)
    {
      ++m_Statistics.m_uiSplitBarrierCount;
    }
  }

  for (const xiiRenderGraph::ResourceEntry& resourceEntry : m_Resources)
  {
    if (!resourceEntry.m_bIsTransient)
      continue;

    if (resourceEntry.m_bIsTexture)
    {
      ++m_Statistics.m_uiTransientTextureCount;
    }
    else
    {
      ++m_Statistics.m_uiTransientBufferCount;
    }
  }

  return XII_SUCCESS;
}

void xiiRenderGraph::PhaseB_TopologicalSortAndCull(const xiiRGCompileSettings& settings, xiiDynamicArray<xiiUInt32>& out_sortedIndices)
{
  const xiiUInt32 uiPassCount = m_Passes.GetCount();
  if (uiPassCount == 0U)
    return;

  // For each resource, track which pass last wrote it (current version -> pass index).
  // Build dependency arcs: for each pass p, if it reads version v of resource r, and version v was produced by pass q, then p depends on q.

  // Use a per-pass in-degree counter and adjacency list.
  xiiDynamicArray<xiiUInt32> inDegree;
  inDegree.SetCount(uiPassCount, 0U);

  xiiDynamicArray<xiiHybridArray<xiiUInt32, 4>> adjacency;
  adjacency.SetCount(uiPassCount);

  // Build producer map: (resource index, version) -> pass index
  xiiHashTable<xiiUInt64, xiiUInt32> producerMap; // Key = resource index | (version << 32).
  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    for (const ResourceUsage& write : m_Passes[uiPassIndex].m_Writes)
    {
      const xiiUInt64 uiKey = static_cast<xiiUInt64>(write.m_uiResourceIndex) | (static_cast<xiiUInt64>(write.m_uiVersion) << 32ULL);

      producerMap.Insert(uiKey, uiPassIndex);
    }
  }

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    for (const ResourceUsage& read : m_Passes[uiPassIndex].m_Reads)
    {
      const xiiUInt64 uiKey               = static_cast<xiiUInt64>(read.m_uiResourceIndex) | (static_cast<xiiUInt64>(read.m_uiVersion) << 32ULL);
      xiiUInt32       uiProducerPassIndex = xiiInvalidIndex;

      if (producerMap.TryGetValue(uiKey, uiProducerPassIndex) && uiProducerPassIndex != uiPassIndex)
      {
        adjacency[uiProducerPassIndex].PushBack(uiPassIndex);

        ++inDegree[uiPassIndex];
      }
    }
  }

  // Kahn's algorithm.
  xiiDeque<xiiUInt32> readyQueue;
  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    if (inDegree[uiPassIndex] == 0U)
    {
      readyQueue.PushBack(uiPassIndex);
    }
  }

  out_sortedIndices.Reserve(uiPassCount);
  while (!readyQueue.IsEmpty())
  {
    const xiiUInt32 uiCurrentIndex = readyQueue.PeekFront();

    readyQueue.PopFront();
    out_sortedIndices.PushBack(uiCurrentIndex);

    for (xiiUInt32 uiSuccessorIndex : adjacency[uiCurrentIndex])
    {
      if (--inDegree[uiSuccessorIndex] == 0U)
      {
        readyQueue.PushBack(uiSuccessorIndex);
      }
    }
  }

  XII_ASSERT_DEV(out_sortedIndices.GetCount() == uiPassCount, "Render graph has a dependency cycle ({} of {} passes sorted).", out_sortedIndices.GetCount(), uiPassCount);

  if (!settings.m_bEnablePassCulling)
  {
    // Build CompiledPasses without culling.
    for (xiiUInt32 uiIndex : out_sortedIndices)
    {
      xiiRGCompiledPass& compiledPass = m_CompiledPasses.ExpandAndGetRef();
      compiledPass.m_sName            = m_Passes[uiIndex].m_sName;
      compiledPass.m_uiPassIndex      = uiIndex;
      compiledPass.m_uiQueueIndex     = 0U;
      compiledPass.m_bHasSideEffects  = m_Passes[uiIndex].m_bHasSideEffects;
      compiledPass.m_bAllowMerge      = m_Passes[uiIndex].m_bAllowMerge;
      compiledPass.m_bIsCulled        = false;
      compiledPass.m_pPassData        = m_Passes[uiIndex].m_pPassData;
      compiledPass.m_ExecuteDelegate  = m_Passes[uiIndex].m_ExecuteDelegate;
    }

    m_Statistics.m_uiCulledPassCount = 0U;
    return;
  }

  // Backward reachability from side-effect passes.
  xiiDynamicArray<bool> isLive;
  isLive.SetCount(uiPassCount, false);

  // Build reverse adjacency.
  xiiDynamicArray<xiiHybridArray<xiiUInt32, 4>> reverseAdjacency;
  reverseAdjacency.SetCount(uiPassCount);

  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    for (xiiUInt32 uiSuccessor : adjacency[uiPassIndex])
    {
      reverseAdjacency[uiSuccessor].PushBack(uiPassIndex);
    }
  }

  xiiDeque<xiiUInt32> workList;
  for (xiiUInt32 uiPassIndex = 0U; uiPassIndex < uiPassCount; ++uiPassIndex)
  {
    if (m_Passes[uiPassIndex].m_bHasSideEffects)
    {
      isLive[uiPassIndex] = true;

      workList.PushBack(uiPassIndex);
    }
  }

  while (!workList.IsEmpty())
  {
    const xiiUInt32 uiCurrentIndex = workList.PeekFront();

    workList.PopFront();

    for (xiiUInt32 uiPredecessor : reverseAdjacency[uiCurrentIndex])
    {
      if (!isLive[uiPredecessor])
      {
        isLive[uiPredecessor] = true;

        workList.PushBack(uiPredecessor);
      }
    }
  }

  xiiUInt32 uiCullCount = 0U;
  for (xiiUInt32 uiIndex : out_sortedIndices)
  {
    xiiRGCompiledPass& compiledPass = m_CompiledPasses.ExpandAndGetRef();
    compiledPass.m_sName            = m_Passes[uiIndex].m_sName;
    compiledPass.m_uiPassIndex      = uiIndex;
    compiledPass.m_uiQueueIndex     = 0U;
    compiledPass.m_bHasSideEffects  = m_Passes[uiIndex].m_bHasSideEffects;
    compiledPass.m_bAllowMerge      = m_Passes[uiIndex].m_bAllowMerge;
    compiledPass.m_bIsCulled        = !isLive[uiIndex];
    compiledPass.m_pPassData        = m_Passes[uiIndex].m_pPassData;
    compiledPass.m_ExecuteDelegate  = m_Passes[uiIndex].m_ExecuteDelegate;

    if (compiledPass.m_bIsCulled)
    {
      ++uiCullCount;
    }
  }
  m_Statistics.m_uiCulledPassCount = uiCullCount;
}

void xiiRenderGraph::PhaseC_LifetimeAnalysis(const xiiDynamicArray<xiiUInt32>& sortedIndices)
{
  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < sortedIndices.GetCount(); ++uiSortedIndex)
  {
    const xiiUInt32          uiPassIndex  = sortedIndices[uiSortedIndex];
    const PassEntry&         pass         = m_Passes[uiPassIndex];
    const xiiRGCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];

    if (compiledPass.m_bIsCulled)
      continue;

    auto UpdateLifetime = [&](xiiUInt32 uiResourceIndex) -> void {
      ResourceEntry& resourceEntry = m_Resources[uiResourceIndex];

      if (resourceEntry.m_uiFirstUsePassIdx == xiiInvalidIndex)
      {
        resourceEntry.m_uiFirstUsePassIdx = uiSortedIndex;
      }
      resourceEntry.m_uiLastUsePassIdx = uiSortedIndex;
    };

    for (const ResourceUsage& read : pass.m_Reads)
    {
      UpdateLifetime(read.m_uiResourceIndex);
    }
    for (const ResourceUsage& write : pass.m_Writes)
    {
      UpdateLifetime(write.m_uiResourceIndex);
    }
  }

  // Map acquire/release to compiled pass entries.
  for (xiiUInt32 uiResourceIndex = 0U; uiResourceIndex < m_Resources.GetCount(); ++uiResourceIndex)
  {
    const ResourceEntry& resourceEntry = m_Resources[uiResourceIndex];
    if (!resourceEntry.m_bIsTransient || resourceEntry.m_uiFirstUsePassIdx == xiiInvalidIndex)
      continue;

    m_CompiledPasses[resourceEntry.m_uiFirstUsePassIdx].m_AcquireResourceIndices.PushBack(uiResourceIndex);
    m_CompiledPasses[resourceEntry.m_uiLastUsePassIdx].m_ReleaseResourceIndices.PushBack(uiResourceIndex);
  }
}

// static
xiiBitflags<xiiGALResourceStateFlags> xiiRenderGraph::InferStateFromUsage(const ResourceUsage& usage)
{
  // We currently explicitly states via the builder, so use them directly.
  // This helper exists for future heuristics.
  return usage.m_RequiredState;
}

void xiiRenderGraph::EmitBarrier(xiiUInt32 uiConsumerPassIdx, xiiUInt32 uiResourceIdx, bool bIsTexture, xiiBitflags<xiiGALResourceStateFlags> afterState, bool bSplitBarrier, xiiUInt32 uiFirstMip, xiiUInt32 uiMipCount, xiiUInt32 uiFirstSlice, xiiUInt32 uiSliceCount)
{
  ResourceEntry&                              resourceEntry = m_Resources[uiResourceIdx];
  const xiiBitflags<xiiGALResourceStateFlags> beforeState   = resourceEntry.m_CurrentState;

  // Don't emit a no-op barrier.
  if (beforeState == afterState && beforeState != xiiGALResourceStateFlags::UnorderedAccess)
    return;

  xiiRGBarrierDescription barrier;
  barrier.m_uiResourceIndex   = uiResourceIdx;
  barrier.m_bIsTexture        = bIsTexture;
  barrier.m_uiFirstMipLevel   = uiFirstMip;
  barrier.m_uiMipLevelCount   = uiMipCount;
  barrier.m_uiFirstArraySlice = uiFirstSlice;
  barrier.m_uiArraySliceCount = uiSliceCount;
  barrier.m_BeforeState       = beforeState;
  barrier.m_AfterState        = afterState;
  barrier.m_TransitionFlags   = xiiGALStateTransitionFlags::UpdateState;

  const xiiUInt32 uiBarrierIndex = m_Barriers.GetCount();

  if (bSplitBarrier)
  {
    // Producer emits Begin, consumer emits End.
    barrier.m_TransitionType = xiiGALStateTransitionType::Begin;
    m_Barriers.PushBack(barrier);

    // Find the producer pass compiled index.
    const xiiUInt32 uiProducerPassIndex = resourceEntry.m_uiCurrentProducerPassIdx;
    for (xiiRGCompiledPass& compiledPass : m_CompiledPasses)
    {
      if (compiledPass.m_uiPassIndex == uiProducerPassIndex && !compiledPass.m_bIsCulled)
      {
        compiledPass.m_PostBarrierBeginIndices.PushBack(uiBarrierIndex);
        break;
      }
    }

    // End barrier goes on consumer.
    xiiRGBarrierDescription endBarrier = barrier;
    endBarrier.m_TransitionType        = xiiGALStateTransitionType::End;
    const xiiUInt32 uiEndBarrierIndex  = m_Barriers.GetCount();
    m_Barriers.PushBack(endBarrier);

    for (xiiRGCompiledPass& compiledPass : m_CompiledPasses)
    {
      if (compiledPass.m_uiPassIndex == uiConsumerPassIdx && !compiledPass.m_bIsCulled)
      {
        compiledPass.m_PreBarrierIndices.PushBack(uiEndBarrierIndex);
        break;
      }
    }
  }
  else
  {
    barrier.m_TransitionType = xiiGALStateTransitionType::Immediate;
    m_Barriers.PushBack(barrier);

    for (xiiRGCompiledPass& compiledPass : m_CompiledPasses)
    {
      if (compiledPass.m_uiPassIndex == uiConsumerPassIdx && !compiledPass.m_bIsCulled)
      {
        compiledPass.m_PreBarrierIndices.PushBack(uiBarrierIndex);
        break;
      }
    }
  }

  // Advance the resource's known state.
  resourceEntry.m_CurrentState = afterState;
}

void xiiRenderGraph::PhaseD_BarrierSynthesis(const xiiDynamicArray<xiiUInt32>& sortedIndices, const xiiRGCompileSettings& settings)
{
  // Initialize resource states from imported resources.
  for (ResourceEntry& resourceEntry : m_Resources)
  {
    if (resourceEntry.m_bIsImported)
    {
      resourceEntry.m_CurrentState = resourceEntry.m_ImportedInitialState;
    }
    else
    {
      resourceEntry.m_CurrentState = xiiGALResourceStateFlags::Unknown;
    }
  }

  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < sortedIndices.GetCount(); ++uiSortedIndex)
  {
    const xiiUInt32          uiPassIndex  = sortedIndices[uiSortedIndex];
    const xiiRGCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];

    if (compiledPass.m_bIsCulled)
      continue;

    const PassEntry& pass = m_Passes[uiPassIndex];

    for (const ResourceUsage& usage : pass.m_Reads)
    {
      const xiiUInt32                             uiResourceIndex = usage.m_uiResourceIndex;
      ResourceEntry&                              resourceEntry   = m_Resources[uiResourceIndex];
      const xiiBitflags<xiiGALResourceStateFlags> requiredState   = usage.m_RequiredState;

      if (resourceEntry.m_CurrentState == requiredState)
        continue;

      // UAV barrier: read from unordered access after write to unordered access.
      const bool bUAVBarrier = (resourceEntry.m_CurrentState == xiiGALResourceStateFlags::UnorderedAccess && requiredState == xiiGALResourceStateFlags::UnorderedAccess);

      // Only emit UAV barriers as Immediate (spec requires it).
      const bool bSplit = settings.m_bEnableSplitBarriers && !bUAVBarrier && (resourceEntry.m_uiCurrentProducerPassIdx != xiiInvalidIndex);

      EmitBarrier(uiPassIndex, uiResourceIndex, usage.m_bIsTexture, requiredState, bSplit);
    }

    for (const ResourceUsage& usage : pass.m_Writes)
    {
      const xiiUInt32                             uiResourceIndex = usage.m_uiResourceIndex;
      ResourceEntry&                              resourceEntry   = m_Resources[uiResourceIndex];
      const xiiBitflags<xiiGALResourceStateFlags> requiredState   = usage.m_RequiredState;

      if (resourceEntry.m_CurrentState == requiredState && requiredState != xiiGALResourceStateFlags::UnorderedAccess)
        continue;

      const bool bSplit = settings.m_bEnableSplitBarriers && (resourceEntry.m_uiCurrentProducerPassIdx != xiiInvalidIndex) && (requiredState != xiiGALResourceStateFlags::UnorderedAccess);

      EmitBarrier(uiPassIndex, uiResourceIndex, usage.m_bIsTexture, requiredState, bSplit);
    }
  }
}

void xiiRenderGraph::PhaseE_MultiQueueScheduling(const xiiDynamicArray<xiiUInt32>& sortedIndices, xiiGALDevice* /*pDevice*/, const xiiRGCompileSettings& settings)
{
  // Map queue flags to a queue index (0=Graphics, 1=Compute, 2=Transfer).
  auto QueueFlagsToIndex = [](xiiBitflags<xiiGALCommandQueueFlags> flags) -> xiiUInt32 {
    if (flags.IsSet(xiiGALCommandQueueFlags::Transfer) && !flags.IsSet(xiiGALCommandQueueFlags::Graphics))
      return 2U;
    if (flags.IsSet(xiiGALCommandQueueFlags::Compute) && !flags.IsSet(xiiGALCommandQueueFlags::Graphics))
      return 1U;
    return 0U;
  };

  // Identify distinct queue flags in use.
  static constexpr xiiUInt32                                          s_uiMaxQueues = 3U;
  xiiStaticArray<xiiBitflags<xiiGALCommandQueueFlags>, s_uiMaxQueues> queueFlags;
  queueFlags.SetCount(s_uiMaxQueues);

  queueFlags[0U] = xiiGALCommandQueueFlags::Graphics;
  queueFlags[1U] = xiiGALCommandQueueFlags::Compute;
  queueFlags[2U] = xiiGALCommandQueueFlags::Transfer;

  // Assign queue index to each non-culled compiled pass.
  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < sortedIndices.GetCount(); ++uiSortedIndex)
  {
    const xiiUInt32    uiPassIndex  = sortedIndices[uiSortedIndex];
    xiiRGCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];
    if (compiledPass.m_bIsCulled)
      continue;

    const xiiBitflags<xiiGALCommandQueueFlags> queueFlags = m_Passes[uiPassIndex].m_QueueFlags;
    compiledPass.m_uiQueueIndex                           = (settings.m_bEnableAsyncQueues) ? QueueFlagsToIndex(queueFlags) : 0U;
  }

  // Build one xiiRGQueueSubmission per contiguous run of same-queue non-culled passes.
  xiiUInt32             uiCurrentQueue          = xiiInvalidIndex;
  xiiRGQueueSubmission* pCurrentQueueSubmission = nullptr;

  for (xiiUInt32 uiSortedIndex = 0U; uiSortedIndex < m_CompiledPasses.GetCount(); ++uiSortedIndex)
  {
    xiiRGCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];
    if (compiledPass.m_bIsCulled)
      continue;

    if (compiledPass.m_uiQueueIndex != uiCurrentQueue)
    {
      pCurrentQueueSubmission                 = &m_QueueSubmissions.ExpandAndGetRef();
      pCurrentQueueSubmission->m_uiQueueIndex = compiledPass.m_uiQueueIndex;
      pCurrentQueueSubmission->m_QueueFlags   = queueFlags[compiledPass.m_uiQueueIndex < s_uiMaxQueues ? compiledPass.m_uiQueueIndex : 0U];
      uiCurrentQueue                          = compiledPass.m_uiQueueIndex;
    }
    pCurrentQueueSubmission->m_PassOrder.PushBack(uiSortedIndex);
  }

  m_Statistics.m_uiQueueSubmissionCount = m_QueueSubmissions.GetCount();
}

void xiiRenderGraph::PhaseF_RenderPassMerging(xiiGALDevice* pDevice)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null for render pass merging.");
  m_MergeGroups.Clear();

  // Scan compiled passes for consecutive graphics-queue mergeable passes that all write only render-targets / depth-stencil.
  auto IsRTOrDepth = [](xiiBitflags<xiiGALResourceStateFlags> state) -> bool {
    return state.IsAnySet(xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead);
  };

  auto CanMerge = [&](const xiiRGCompiledPass& compiledPass) -> bool {
    if (compiledPass.m_bIsCulled || !compiledPass.m_bAllowMerge || compiledPass.m_uiQueueIndex != 0U)
      return false;

    const PassEntry& passEntry = m_Passes[compiledPass.m_uiPassIndex];
    for (const ResourceUsage& write : passEntry.m_Writes)
    {
      if (!IsRTOrDepth(write.m_RequiredState))
        return false;
    }
    return true;
  };

  const xiiUInt32 uiCount = m_CompiledPasses.GetCount();
  xiiUInt32       i       = 0U;
  while (i < uiCount)
  {
    if (!CanMerge(m_CompiledPasses[i]))
    {
      ++i;
      continue;
    }

    // Start a merge group.
    xiiRGMergeGroup& group        = m_MergeGroups.ExpandAndGetRef();
    const xiiUInt32  uiGroupIndex = m_MergeGroups.GetCount() - 1U;

    while (i < uiCount && CanMerge(m_CompiledPasses[i]))
    {
      group.m_PassIndices.PushBack(i);

      m_CompiledPasses[i].m_uiMergeGroupIndex = uiGroupIndex;

      ++i;
    }

    // Groups of size 1 get no native render pass object, no benefit.
    if (group.m_PassIndices.GetCount() < 2U)
    {
      m_CompiledPasses[group.m_PassIndices[0]].m_uiMergeGroupIndex = xiiInvalidIndex;

      m_MergeGroups.PopBack();
    }
  }

  m_Statistics.m_uiMergeGroupCount = m_MergeGroups.GetCount();
}

// static
xiiUInt64 xiiRenderGraph::ComputeSignature(const xiiDynamicArray<PassEntry>& passes)
{
  xiiUInt64 uiHash = 0x9E3779B97F4A7C15ULL;

  for (const PassEntry& passEntry : passes)
  {
    uiHash = xiiHashingUtils::xxHash64(&passEntry.m_QueueFlags, sizeof(passEntry.m_QueueFlags), uiHash);
    uiHash = xiiHashingUtils::xxHash64String(passEntry.m_sName.GetView(), uiHash);
    uiHash = xiiHashingUtils::xxHash64(&passEntry.m_bHasSideEffects, sizeof(bool), uiHash);

    for (const ResourceUsage& read : passEntry.m_Reads)
    {
      uiHash = xiiHashingUtils::xxHash64(&read, sizeof(read), uiHash);
    }
    for (const ResourceUsage& write : passEntry.m_Writes)
    {
      uiHash = xiiHashingUtils::xxHash64(&write, sizeof(write), uiHash);
    }
  }
  return uiHash;
}

void xiiRenderGraph::PhaseG_SignatureAndCache(const xiiRGCompileSettings& settings)
{
  const xiiUInt64 uiSignature     = ComputeSignature(m_Passes) ^ static_cast<xiiUInt64>(settings.m_uiCacheSalt);
  m_Statistics.m_uiGraphSignature = uiSignature;

  if (settings.m_bEnableCompileCache && uiSignature == m_uiLastSignature && m_bIsCompiled)
  {
    m_Statistics.m_bUsedCachedCompile = true;
  }
  else
  {
    m_uiLastSignature                 = uiSignature;
    m_Statistics.m_bUsedCachedCompile = false;
  }
}

xiiResult xiiRenderGraph::Execute(xiiGALDevice* pDevice, const xiiView* pView, xiiRenderGraphBlackboard* pBlackboard, xiiRenderGraphResourceCache* pResourceCache, xiiRenderGraphProfiler* pProfiler, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null.");
  XII_ASSERT_DEV(pBlackboard != nullptr, "Blackboard must not be null.");
  XII_ASSERT_DEV(pResourceCache != nullptr, "ResourceCache must not be null.");

  if (!m_bIsCompiled)
  {
    if (out_pError)
    {
      *out_pError = "Execute called on an uncompiled render graph.";
    }
    return XII_FAILURE;
  }

  // Run render-pass merging now that we have a live device.
  PhaseF_RenderPassMerging(pDevice);

  // Resolve all transient and imported resources for this frame's execution.
  const xiiUInt32                              uiResourceCount = m_Resources.GetCount();
  xiiDynamicArray<xiiSharedPtr<xiiGALTexture>> resolvedTextures;
  xiiDynamicArray<xiiSharedPtr<xiiGALBuffer>>  resolvedBuffers;
  resolvedTextures.SetCount(uiResourceCount);
  resolvedBuffers.SetCount(uiResourceCount);

  for (xiiUInt32 i = 0U; i < uiResourceCount; ++i)
  {
    ResourceEntry& resourceEntry = m_Resources[i];

    if (resourceEntry.m_bIsImported)
    {
      if (resourceEntry.m_bIsTexture)
      {
        resolvedTextures[i] = resourceEntry.m_pImportedTexture;
      }
      else
      {
        resolvedBuffers[i] = resourceEntry.m_pImportedBuffer;
      }
    }
  }

  // Execute per queue submission.
  for (const xiiRGQueueSubmission& submission : m_QueueSubmissions)
  {
    xiiGALCommandQueue* pQueue = pDevice->GetCommandQueue(submission.m_QueueFlags);
    XII_ASSERT_DEV(pQueue != nullptr, "Could not obtain a command queue.");

    // Create a command list for this submission.
    xiiGALCommandListCreationDescription commandListDescription;
    commandListDescription.m_QueueFlags          = submission.m_QueueFlags;
    xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(commandListDescription);
    XII_ASSERT_ALWAYS(pCommandList != nullptr, "Failed to create command list.");

    pCommandList->Begin();

    // Emit cross-queue waits.
    for (xiiUInt32 uiWaitIndex = 0U; uiWaitIndex < submission.m_WaitFences.GetCount(); ++uiWaitIndex)
    {
      pCommandList->DeviceWaitForFence(submission.m_WaitFences[uiWaitIndex].Borrow(), submission.m_WaitValues[uiWaitIndex]);
    }

    xiiUInt32 uiCurrentMergeGroup = xiiInvalidIndex;
    bool      bInsideRenderPass   = false;

    for (xiiUInt32 uiSortedIndex : submission.m_PassOrder)
    {
      xiiRGCompiledPass& compiledPass = m_CompiledPasses[uiSortedIndex];
      if (compiledPass.m_bIsCulled)
        continue;

      // Acquire transient resources whose lifetime starts at this pass.
      for (xiiUInt32 uiResourceIndex : compiledPass.m_AcquireResourceIndices)
      {
        ResourceEntry& resourceEntry = m_Resources[uiResourceIndex];

        if (resourceEntry.m_bIsTexture)
        {
          resolvedTextures[uiResourceIndex] = pResourceCache->AcquireTexture(resourceEntry.m_TextureDescription);
        }
        else
        {
          resolvedBuffers[uiResourceIndex] = pResourceCache->AcquireBuffer(resourceEntry.m_BufferDescription);
        }
      }

      // Emit pre-barriers (split-bar ends + immediate barriers).
      if (!compiledPass.m_PreBarrierIndices.IsEmpty())
      {
        xiiSmallArray<xiiGALStateTransitionDescription, 8> transitions;

        for (xiiUInt32 uiBarrierIndex : compiledPass.m_PreBarrierIndices)
        {
          const xiiRGBarrierDescription&    barrier               = m_Barriers[uiBarrierIndex];
          xiiGALStateTransitionDescription& transitionDescription = transitions.ExpandAndGetRef();

          if (barrier.m_bIsTexture)
          {
            transitionDescription.m_pResource = resolvedTextures[barrier.m_uiResourceIndex];
          }
          else
          {
            transitionDescription.m_pResource = resolvedBuffers[barrier.m_uiResourceIndex];
          }

          transitionDescription.m_OldState          = barrier.m_BeforeState;
          transitionDescription.m_NewState          = barrier.m_AfterState;
          transitionDescription.m_TransitionType    = barrier.m_TransitionType;
          transitionDescription.m_TransitionFlags   = barrier.m_TransitionFlags;
          transitionDescription.m_uiFirstMipLevel   = barrier.m_uiFirstMipLevel;
          transitionDescription.m_uiMipLevelCount   = barrier.m_uiMipLevelCount;
          transitionDescription.m_uiFirstArraySlice = barrier.m_uiFirstArraySlice;
          transitionDescription.m_uiArraySliceCount = barrier.m_uiArraySliceCount;
        }

        pCommandList->TransitionResourceStates(transitions);
      }

      // Handle merge group open.
      if (compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex && compiledPass.m_uiMergeGroupIndex != uiCurrentMergeGroup)
      {
        xiiRGMergeGroup& mergeGroup = m_MergeGroups[compiledPass.m_uiMergeGroupIndex];

        if (mergeGroup.m_pNativeRenderPass != nullptr && mergeGroup.m_pFramebuffer != nullptr)
        {
          xiiGALBeginRenderPassDescription renderPassDescription;
          renderPassDescription.m_pRenderPass  = mergeGroup.m_pNativeRenderPass;
          renderPassDescription.m_pFramebuffer = mergeGroup.m_pFramebuffer;

          pCommandList->BeginRenderPass(renderPassDescription);

          bInsideRenderPass = true;
        }

        uiCurrentMergeGroup = compiledPass.m_uiMergeGroupIndex;
      }
      else if (compiledPass.m_uiMergeGroupIndex == xiiInvalidIndex && bInsideRenderPass)
      {
        pCommandList->EndRenderPass();

        bInsideRenderPass   = false;
        uiCurrentMergeGroup = xiiInvalidIndex;
      }

      // Profiler begin.
      if (pProfiler && m_LastCompileSettings.m_bEnableGPUProfiling)
      {
        pProfiler->OnPassBegin(*pCommandList, compiledPass.m_sName, compiledPass.m_uiPassIndex);
      }

      // Execute pass.
      xiiRGPassContext context;
      context.m_pCommandList     = pCommandList.Borrow();
      context.m_pBlackboard      = pBlackboard;
      context.m_pResourceCache   = pResourceCache;
      context.m_pView            = pView;
      context.m_uiFrameIndex     = m_uiFrameIndex;
      context.m_sPassName        = compiledPass.m_sName;
      context.m_ResolvedTextures = resolvedTextures;
      context.m_ResolvedBuffers  = resolvedBuffers;

      compiledPass.m_ExecuteDelegate(context);

      // Profiler end.
      if (pProfiler && m_LastCompileSettings.m_bEnableGPUProfiling)
      {
        pProfiler->OnPassEnd(*pCommandList, compiledPass.m_sName, compiledPass.m_uiPassIndex);
      }

      // Close merge group if last pass in group.
      if (bInsideRenderPass && compiledPass.m_uiMergeGroupIndex != xiiInvalidIndex)
      {
        const xiiRGMergeGroup& mergeGroup = m_MergeGroups[compiledPass.m_uiMergeGroupIndex];

        if (!mergeGroup.m_PassIndices.IsEmpty() && mergeGroup.m_PassIndices.PeekBack() == uiSortedIndex)
        {
          pCommandList->EndRenderPass();

          bInsideRenderPass   = false;
          uiCurrentMergeGroup = xiiInvalidIndex;
        }
      }

      // Emit split-barrier begins (post-pass).
      if (!compiledPass.m_PostBarrierBeginIndices.IsEmpty())
      {
        xiiSmallArray<xiiGALStateTransitionDescription, 8> transitions;

        for (xiiUInt32 uiBarrierIndex : compiledPass.m_PostBarrierBeginIndices)
        {
          const xiiRGBarrierDescription&    barrier               = m_Barriers[uiBarrierIndex];
          xiiGALStateTransitionDescription& transitionDescription = transitions.ExpandAndGetRef();

          if (barrier.m_bIsTexture)
          {
            transitionDescription.m_pResource = resolvedTextures[barrier.m_uiResourceIndex];
          }
          else
          {
            transitionDescription.m_pResource = resolvedBuffers[barrier.m_uiResourceIndex];
          }

          transitionDescription.m_OldState        = barrier.m_BeforeState;
          transitionDescription.m_NewState        = barrier.m_AfterState;
          transitionDescription.m_TransitionType  = xiiGALStateTransitionType::Begin;
          transitionDescription.m_TransitionFlags = barrier.m_TransitionFlags;
        }
        pCommandList->TransitionResourceStates(transitions);
      }

      // Release transient resources whose lifetime ends at this pass.
      for (xiiUInt32 uiResourceIndex : compiledPass.m_ReleaseResourceIndices)
      {
        ResourceEntry& resourceEntry = m_Resources[uiResourceIndex];

        if (resourceEntry.m_bIsTexture && resolvedTextures[uiResourceIndex])
        {
          pResourceCache->ReturnTexture(resolvedTextures[uiResourceIndex]);

          resolvedTextures[uiResourceIndex] = nullptr;
        }
        else if (!resourceEntry.m_bIsTexture && resolvedBuffers[uiResourceIndex])
        {
          pResourceCache->ReturnBuffer(resolvedBuffers[uiResourceIndex]);

          resolvedBuffers[uiResourceIndex] = nullptr;
        }
      }
    } // per-pass loop

    // Close any still-open render pass.
    if (bInsideRenderPass)
    {
      pCommandList->EndRenderPass();

      bInsideRenderPass = false;
    }

    // Emit cross-queue signal.
    if (submission.m_pSignalFence != nullptr)
    {
      pCommandList->EnqueueSignal(submission.m_pSignalFence, submission.m_uiSignalValue);
    }

    pCommandList->End();

    pQueue->Submit(pCommandList);
  } // per-submission loop

  // Notify profiler.
  if (pProfiler)
  {
    pProfiler->OnFrameEnd(m_uiFrameIndex);
  }

  return XII_SUCCESS;
}

xiiResult xiiRenderGraph::DumpToDot(xiiStringBuilder& out_sDot) const
{
  if (!m_bIsCompiled)
    return XII_FAILURE;

  return xiiRenderGraphDebug::DumpToDot(m_CompiledPasses, m_Barriers, m_MergeGroups, m_QueueSubmissions, out_sDot);
}
