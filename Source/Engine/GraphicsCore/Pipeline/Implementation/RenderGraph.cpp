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
  m_Graph.m_ResourceNameIndex.Insert(sName, uiTextureResourceIndex);

  xiiRGTextureHandle hTexture;
  hTexture.m_uiIndex   = uiTextureResourceIndex;
  hTexture.m_uiVersion = 0U;
  return hTexture;
}

xiiRGTextureHandle xiiRGBuilder::ImportTexture(xiiStringView sName, xiiSharedPtr<xiiGALTexture> pTexture, xiiBitflags<xiiGALResourceStateFlags> currentState)
{
  XII_ASSERT_DEV(pTexture != nullptr, "Cannot import a null texture.");

  xiiUInt32 uiTextureResourceIndex = xiiInvalidIndex;
  if (!m_Graph.m_ResourceNameIndex.TryGetValue(sName, uiTextureResourceIndex))
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
    m_Graph.m_ResourceNameIndex.Insert(sName, uiTextureResourceIndex);
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
  if (m_Graph.m_ResourceNameIndex.TryGetValue(sName, uiBufferResourceIndex))
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
  m_Graph.m_ResourceNameIndex.Insert(sName, uiBufferResourceIndex);

  xiiRGBufferHandle hBuffer;
  hBuffer.m_uiIndex   = uiBufferResourceIndex;
  hBuffer.m_uiVersion = 0U;
  return hBuffer;
}

xiiRGBufferHandle xiiRGBuilder::ImportBuffer(xiiStringView sName, xiiSharedPtr<xiiGALBuffer> pBuffer, xiiBitflags<xiiGALResourceStateFlags> currentState)
{
  XII_ASSERT_DEV(pBuffer != nullptr, "Cannot import a null buffer.");

  xiiUInt32 uiBufferResourceIndex = xiiInvalidIndex;
  if (!m_Graph.m_ResourceNameIndex.TryGetValue(sName, uiBufferResourceIndex))
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
    m_Graph.m_ResourceNameIndex.Insert(sName, uiBufferResourceIndex);
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
  resourceEntry.m_uiCurrentProducerPassIdx = m_uiPassIndex;
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
    if (entry.m_pPassData && entry.m_pfnDestroyPassData)
    {
      entry.m_pfnDestroyPassData(entry.m_pPassData);
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
    if (entry.m_pPassData && entry.m_pfnDestroyPassData)
      entry.m_pfnDestroyPassData(entry.m_pPassData);
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

// ----------------------------------------------------------------------------
//  Compile — main entry
// ----------------------------------------------------------------------------

xiiResult xiiRenderGraph::Compile(const xiiRGCompileSettings& settings, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(!m_bIsSetupOpen, "Cannot compile while setup is still open.");

  m_CompiledPasses.Clear();
  m_Barriers.Clear();
  m_MergeGroups.Clear();
  m_QueueSubmissions.Clear();
  m_Statistics                         = {};
  m_Statistics.m_uiRegisteredPassCount = m_Passes.GetCount();

  // Phase G pre-check: compute signature; if unchanged and cache enabled → skip recompile.
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
  // Pass nullptr device here — queue checks happen at Execute time.
  PhaseE_MultiQueueScheduling(sortedIndices, nullptr, settings);

  // Phase F: render-pass merging (device needed to create native render passes;
  //          deferred to Execute for the first frame, then cached).
  // Merging is completed during Execute once a device is available.

  m_bIsCompiled                         = true;
  m_Statistics.m_uiCompiledPassCount    = m_CompiledPasses.GetCount();
  m_Statistics.m_uiTotalBarrierCount    = m_Barriers.GetCount();
  m_Statistics.m_uiQueueSubmissionCount = m_QueueSubmissions.GetCount();

  for (const xiiRGBarrierDesc& b : m_Barriers)
    if (b.m_TransitionType != xiiGALStateTransitionType::Immediate)
      ++m_Statistics.m_uiSplitBarrierCount;

  for (const xiiRenderGraph::ResourceEntry& res : m_Resources)
  {
    if (!res.m_bIsTransient) continue;
    if (res.m_bIsTexture) ++m_Statistics.m_uiTransientTextureCount;
    else
      ++m_Statistics.m_uiTransientBufferCount;
  }

  return XII_SUCCESS;
}

// ----------------------------------------------------------------------------
//  Phase B — topological sort + live-pass culling (Kahn's algorithm)
// ----------------------------------------------------------------------------

void xiiRenderGraph::PhaseB_TopologicalSortAndCull(const xiiRGCompileSettings& settings,
                                                   xiiDynamicArray<xiiUInt32>& out_sortedIndices)
{
  const xiiUInt32 uiPassCount = m_Passes.GetCount();
  if (uiPassCount == 0U)
    return;

  // For each resource, track which pass last wrote it (current version → pass index).
  // Build dependency arcs: for each pass p, if it reads version v of resource r,
  // and version v was produced by pass q, then p depends on q.

  // Use a per-pass in-degree counter and adjacency list.
  xiiDynamicArray<xiiUInt32> inDegree;
  inDegree.SetCount(uiPassCount, 0U);

  xiiDynamicArray<xiiHybridArray<xiiUInt32, 4>> adjacency;
  adjacency.SetCount(uiPassCount);

  // Build producer map: (resourceIdx, version) → passIdx
  xiiHashTable<xiiUInt64, xiiUInt32> producerMap; // key = resourceIdx | (version << 32)
  for (xiiUInt32 p = 0U; p < uiPassCount; ++p)
  {
    for (const ResourceUsage& write : m_Passes[p].m_Writes)
    {
      const xiiUInt64 key = static_cast<xiiUInt64>(write.m_uiResourceIndex) | (static_cast<xiiUInt64>(write.m_uiVersion) << 32ULL);
      producerMap.Insert(key, p);
    }
  }

  for (xiiUInt32 p = 0U; p < uiPassCount; ++p)
  {
    for (const ResourceUsage& read : m_Passes[p].m_Reads)
    {
      const xiiUInt64 key             = static_cast<xiiUInt64>(read.m_uiResourceIndex) | (static_cast<xiiUInt64>(read.m_uiVersion) << 32ULL);
      xiiUInt32       producerPassIdx = xiiInvalidIndex;
      if (producerMap.TryGetValue(key, producerPassIdx) && producerPassIdx != p)
      {
        adjacency[producerPassIdx].PushBack(p);
        ++inDegree[p];
      }
    }
  }

  // Kahn's algorithm.
  xiiDeque<xiiUInt32> readyQueue;
  for (xiiUInt32 p = 0U; p < uiPassCount; ++p)
    if (inDegree[p] == 0U)
      readyQueue.PushBack(p);

  out_sortedIndices.Reserve(uiPassCount);
  while (!readyQueue.IsEmpty())
  {
    const xiiUInt32 cur = readyQueue.PeekFront();
    readyQueue.PopFront();
    out_sortedIndices.PushBack(cur);
    for (xiiUInt32 succ : adjacency[cur])
    {
      if (--inDegree[succ] == 0U)
        readyQueue.PushBack(succ);
    }
  }

  XII_ASSERT_DEV(out_sortedIndices.GetCount() == uiPassCount,
                 "Render graph has a dependency cycle ({} of {} passes sorted).",
                 out_sortedIndices.GetCount(), uiPassCount);

  if (!settings.m_bEnablePassCulling)
  {
    // Build CompiledPasses without culling.
    for (xiiUInt32 idx : out_sortedIndices)
    {
      xiiRGCompiledPass& cp = m_CompiledPasses.ExpandAndGetRef();
      cp.m_sName            = m_Passes[idx].m_sName;
      cp.m_uiPassIndex      = idx;
      cp.m_uiQueueIndex     = 0U;
      cp.m_bHasSideEffects  = m_Passes[idx].m_bHasSideEffects;
      cp.m_bAllowMerge      = m_Passes[idx].m_bAllowMerge;
      cp.m_bIsCulled        = false;
      cp.m_pPassData        = m_Passes[idx].m_pPassData;
      cp.m_ExecuteDelegate  = m_Passes[idx].m_ExecuteDelegate;
    }
    m_Statistics.m_uiCulledPassCount = 0U;
    return;
  }

  // Backward reachability from side-effect passes.
  xiiDynamicArray<bool> isLive;
  isLive.SetCount(uiPassCount, false);

  // Build reverse adjacency.
  xiiDynamicArray<xiiHybridArray<xiiUInt32, 4>> revAdj;
  revAdj.SetCount(uiPassCount);
  for (xiiUInt32 p = 0U; p < uiPassCount; ++p)
    for (xiiUInt32 succ : adjacency[p])
      revAdj[succ].PushBack(p);

  xiiDeque<xiiUInt32> workList;
  for (xiiUInt32 p = 0U; p < uiPassCount; ++p)
    if (m_Passes[p].m_bHasSideEffects)
    {
      isLive[p] = true;
      workList.PushBack(p);
    }

  while (!workList.IsEmpty())
  {
    const xiiUInt32 cur = workList.PeekFront();
    workList.PopFront();
    for (xiiUInt32 pred : revAdj[cur])
    {
      if (!isLive[pred])
      {
        isLive[pred] = true;
        workList.PushBack(pred);
      }
    }
  }

  xiiUInt32 uiCulled = 0U;
  for (xiiUInt32 idx : out_sortedIndices)
  {
    xiiRGCompiledPass& cp = m_CompiledPasses.ExpandAndGetRef();
    cp.m_sName            = m_Passes[idx].m_sName;
    cp.m_uiPassIndex      = idx;
    cp.m_uiQueueIndex     = 0U;
    cp.m_bHasSideEffects  = m_Passes[idx].m_bHasSideEffects;
    cp.m_bAllowMerge      = m_Passes[idx].m_bAllowMerge;
    cp.m_bIsCulled        = !isLive[idx];
    cp.m_pPassData        = m_Passes[idx].m_pPassData;
    cp.m_ExecuteDelegate  = m_Passes[idx].m_ExecuteDelegate;
    if (cp.m_bIsCulled) ++uiCulled;
  }
  m_Statistics.m_uiCulledPassCount = uiCulled;
}

// ----------------------------------------------------------------------------
//  Phase C — transient resource lifetime analysis
// ----------------------------------------------------------------------------

void xiiRenderGraph::PhaseC_LifetimeAnalysis(const xiiDynamicArray<xiiUInt32>& sortedIndices)
{
  for (xiiUInt32 sortedPos = 0U; sortedPos < sortedIndices.GetCount(); ++sortedPos)
  {
    const xiiUInt32          passIdx = sortedIndices[sortedPos];
    const PassEntry&         pass    = m_Passes[passIdx];
    const xiiRGCompiledPass& cp      = m_CompiledPasses[sortedPos];
    if (cp.m_bIsCulled) continue;

    auto UpdateLifetime = [&](xiiUInt32 resIdx) {
      ResourceEntry& res = m_Resources[resIdx];
      if (res.m_uiFirstUsePassIdx == xiiInvalidIndex)
        res.m_uiFirstUsePassIdx = sortedPos;
      res.m_uiLastUsePassIdx = sortedPos;
    };

    for (const ResourceUsage& r : pass.m_Reads) UpdateLifetime(r.m_uiResourceIndex);
    for (const ResourceUsage& w : pass.m_Writes) UpdateLifetime(w.m_uiResourceIndex);
  }

  // Map acquire/release to compiled pass entries.
  for (xiiUInt32 resIdx = 0U; resIdx < m_Resources.GetCount(); ++resIdx)
  {
    const ResourceEntry& res = m_Resources[resIdx];
    if (!res.m_bIsTransient || res.m_uiFirstUsePassIdx == xiiInvalidIndex) continue;

    m_CompiledPasses[res.m_uiFirstUsePassIdx].m_AcquireResourceIndices.PushBack(resIdx);
    m_CompiledPasses[res.m_uiLastUsePassIdx].m_ReleaseResourceIndices.PushBack(resIdx);
  }
}

// ----------------------------------------------------------------------------
//  Phase D — advanced barrier synthesis
// ----------------------------------------------------------------------------

// static
xiiBitflags<xiiGALResourceStateFlags> xiiRenderGraph::InferStateFromUsage(const ResourceUsage& usage)
{
  // The game sets explicit states via the builder, so use them directly.
  // This helper exists for future heuristics.
  return usage.m_RequiredState;
}

void xiiRenderGraph::EmitBarrier(xiiUInt32 uiConsumerPassIdx, xiiUInt32 uiResourceIdx, bool bIsTexture,
                                 xiiBitflags<xiiGALResourceStateFlags> afterState, bool bSplitBarrier,
                                 xiiUInt32 uiFirstMip, xiiUInt32 uiMipCount,
                                 xiiUInt32 uiFirstSlice, xiiUInt32 uiSliceCount)
{
  ResourceEntry&                              res         = m_Resources[uiResourceIdx];
  const xiiBitflags<xiiGALResourceStateFlags> beforeState = res.m_CurrentState;

  // Don't emit a no-op barrier.
  if (beforeState == afterState && beforeState != xiiGALResourceStateFlags::UnorderedAccess)
    return;

  xiiRGBarrierDesc barrier;
  barrier.m_uiResourceIndex   = uiResourceIdx;
  barrier.m_bIsTexture        = bIsTexture;
  barrier.m_uiFirstMipLevel   = uiFirstMip;
  barrier.m_uiMipLevelCount   = uiMipCount;
  barrier.m_uiFirstArraySlice = uiFirstSlice;
  barrier.m_uiArraySliceCount = uiSliceCount;
  barrier.m_BeforeState       = beforeState;
  barrier.m_AfterState        = afterState;
  barrier.m_TransitionFlags   = xiiGALStateTransitionFlags::UpdateState;

  const xiiUInt32 uiBarrierIdx = m_Barriers.GetCount();

  if (bSplitBarrier)
  {
    // Producer emits Begin; consumer emits End.
    barrier.m_TransitionType = xiiGALStateTransitionType::Begin;
    m_Barriers.PushBack(barrier);

    // Find the producer pass compiled index.
    const xiiUInt32 uiProducerPassIdx = res.m_uiCurrentProducerPassIdx;
    for (xiiRGCompiledPass& cp : m_CompiledPasses)
    {
      if (cp.m_uiPassIndex == uiProducerPassIdx && !cp.m_bIsCulled)
      {
        cp.m_PostBarrierBeginIndices.PushBack(uiBarrierIdx);
        break;
      }
    }

    // End barrier goes on consumer.
    xiiRGBarrierDesc endBarrier     = barrier;
    endBarrier.m_TransitionType     = xiiGALStateTransitionType::End;
    const xiiUInt32 uiEndBarrierIdx = m_Barriers.GetCount();
    m_Barriers.PushBack(endBarrier);

    for (xiiRGCompiledPass& cp : m_CompiledPasses)
    {
      if (cp.m_uiPassIndex == uiConsumerPassIdx && !cp.m_bIsCulled)
      {
        cp.m_PreBarrierIndices.PushBack(uiEndBarrierIdx);
        break;
      }
    }
  }
  else
  {
    barrier.m_TransitionType = xiiGALStateTransitionType::Immediate;
    m_Barriers.PushBack(barrier);

    for (xiiRGCompiledPass& cp : m_CompiledPasses)
    {
      if (cp.m_uiPassIndex == uiConsumerPassIdx && !cp.m_bIsCulled)
      {
        cp.m_PreBarrierIndices.PushBack(uiBarrierIdx);
        break;
      }
    }
  }

  // Advance the resource's known state.
  res.m_CurrentState = afterState;
}

void xiiRenderGraph::PhaseD_BarrierSynthesis(const xiiDynamicArray<xiiUInt32>& sortedIndices,
                                             const xiiRGCompileSettings&       settings)
{
  // Initialize resource states from imported resources.
  for (ResourceEntry& res : m_Resources)
  {
    if (res.m_bIsImported)
      res.m_CurrentState = res.m_ImportedInitialState;
    else
      res.m_CurrentState = xiiGALResourceStateFlags::Unknown;
  }

  for (xiiUInt32 sortedPos = 0U; sortedPos < sortedIndices.GetCount(); ++sortedPos)
  {
    const xiiUInt32          passIdx = sortedIndices[sortedPos];
    const xiiRGCompiledPass& cp      = m_CompiledPasses[sortedPos];
    if (cp.m_bIsCulled) continue;

    const PassEntry& pass = m_Passes[passIdx];

    for (const ResourceUsage& usage : pass.m_Reads)
    {
      const xiiUInt32                             resIdx        = usage.m_uiResourceIndex;
      ResourceEntry&                              res           = m_Resources[resIdx];
      const xiiBitflags<xiiGALResourceStateFlags> requiredState = usage.m_RequiredState;

      if (res.m_CurrentState == requiredState) continue;

      // UAV barrier: read from unordered access after write to unordered access.
      const bool bUAVBarrier = (res.m_CurrentState == xiiGALResourceStateFlags::UnorderedAccess && requiredState == xiiGALResourceStateFlags::UnorderedAccess);
      // Only emit UAV barriers as Immediate (spec requires it).
      const bool bSplit = settings.m_bEnableSplitBarriers && !bUAVBarrier && (res.m_uiCurrentProducerPassIdx != xiiInvalidIndex);

      EmitBarrier(passIdx, resIdx, usage.m_bIsTexture, requiredState, bSplit);
    }

    for (const ResourceUsage& usage : pass.m_Writes)
    {
      const xiiUInt32                             resIdx        = usage.m_uiResourceIndex;
      ResourceEntry&                              res           = m_Resources[resIdx];
      const xiiBitflags<xiiGALResourceStateFlags> requiredState = usage.m_RequiredState;

      if (res.m_CurrentState == requiredState && requiredState != xiiGALResourceStateFlags::UnorderedAccess) continue;

      const bool bSplit = settings.m_bEnableSplitBarriers && (res.m_uiCurrentProducerPassIdx != xiiInvalidIndex) && (requiredState != xiiGALResourceStateFlags::UnorderedAccess);

      EmitBarrier(passIdx, resIdx, usage.m_bIsTexture, requiredState, bSplit);
    }
  }
}

// ----------------------------------------------------------------------------
//  Phase E — multi-queue scheduling
// ----------------------------------------------------------------------------

void xiiRenderGraph::PhaseE_MultiQueueScheduling(const xiiDynamicArray<xiiUInt32>& sortedIndices,
                                                 xiiGALDevice* /*pDevice*/, const xiiRGCompileSettings& settings)
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
  static constexpr xiiUInt32                                      MaxQueues = 3U;
  xiiStaticArray<xiiBitflags<xiiGALCommandQueueFlags>, MaxQueues> queueFlags;
  queueFlags[0U] = xiiGALCommandQueueFlags::Graphics;
  queueFlags[1U] = xiiGALCommandQueueFlags::Compute;
  queueFlags[2U] = xiiGALCommandQueueFlags::Transfer;

  // Assign queue index to each non-culled compiled pass.
  for (xiiUInt32 sortedPos = 0U; sortedPos < sortedIndices.GetCount(); ++sortedPos)
  {
    const xiiUInt32    passIdx = sortedIndices[sortedPos];
    xiiRGCompiledPass& cp      = m_CompiledPasses[sortedPos];
    if (cp.m_bIsCulled) continue;

    const xiiBitflags<xiiGALCommandQueueFlags> pf = m_Passes[passIdx].m_QueueFlags;
    cp.m_uiQueueIndex                             = (settings.m_bEnableAsyncQueues) ? QueueFlagsToIndex(pf) : 0U;
  }

  // Build one xiiRGQueueSubmission per contiguous run of same-queue non-culled passes.
  xiiUInt32             uiCurrentQueue = xiiInvalidIndex;
  xiiRGQueueSubmission* pCur           = nullptr;

  for (xiiUInt32 sortedPos = 0U; sortedPos < m_CompiledPasses.GetCount(); ++sortedPos)
  {
    xiiRGCompiledPass& cp = m_CompiledPasses[sortedPos];
    if (cp.m_bIsCulled) continue;

    if (cp.m_uiQueueIndex != uiCurrentQueue)
    {
      pCur                 = &m_QueueSubmissions.ExpandAndGetRef();
      pCur->m_uiQueueIndex = cp.m_uiQueueIndex;
      pCur->m_QueueFlags   = queueFlags[cp.m_uiQueueIndex < MaxQueues ? cp.m_uiQueueIndex : 0U];
      uiCurrentQueue       = cp.m_uiQueueIndex;
    }
    pCur->m_PassOrder.PushBack(sortedPos);
  }

  m_Statistics.m_uiQueueSubmissionCount = m_QueueSubmissions.GetCount();
}

// ----------------------------------------------------------------------------
//  Phase F — render-pass merging (called during Execute with a live device)
// ----------------------------------------------------------------------------

void xiiRenderGraph::PhaseF_RenderPassMerging(xiiGALDevice* pDevice)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null for render pass merging.");
  m_MergeGroups.Clear();

  // Scan compiled passes for consecutive graphics-queue mergeable passes
  // that all write only render-targets / depth-stencil.
  auto IsRTOrDepth = [](xiiBitflags<xiiGALResourceStateFlags> state) -> bool {
    return state.IsAnySet(xiiGALResourceStateFlags::RenderTarget | xiiGALResourceStateFlags::DepthWrite | xiiGALResourceStateFlags::DepthRead);
  };

  auto CanMerge = [&](const xiiRGCompiledPass& cp) -> bool {
    if (cp.m_bIsCulled || !cp.m_bAllowMerge || cp.m_uiQueueIndex != 0U) return false;
    const PassEntry& pass = m_Passes[cp.m_uiPassIndex];
    for (const ResourceUsage& w : pass.m_Writes)
      if (!IsRTOrDepth(w.m_RequiredState)) return false;
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
    xiiRGMergeGroup& group      = m_MergeGroups.ExpandAndGetRef();
    const xiiUInt32  uiGroupIdx = m_MergeGroups.GetCount() - 1U;

    while (i < uiCount && CanMerge(m_CompiledPasses[i]))
    {
      group.m_PassIndices.PushBack(i);
      m_CompiledPasses[i].m_uiMergeGroupIndex = uiGroupIdx;
      ++i;
    }

    // Groups of size 1 get no native render pass object — no benefit.
    if (group.m_PassIndices.GetCount() < 2U)
    {
      m_CompiledPasses[group.m_PassIndices[0]].m_uiMergeGroupIndex = xiiInvalidIndex;
      m_MergeGroups.PopBack();
    }
  }

  m_Statistics.m_uiMergeGroupCount = m_MergeGroups.GetCount();
}

// ----------------------------------------------------------------------------
//  Phase G — signature + compile cache
// ----------------------------------------------------------------------------

// static
xiiUInt64 xiiRenderGraph::ComputeSignature(const xiiDynamicArray<PassEntry>& passes)
{
  xiiUInt64 uiHash = 0x9E3779B97F4A7C15ULL;
  for (const PassEntry& p : passes)
  {
    uiHash = xiiHashingUtils::xxHash64(&p.m_QueueFlags, sizeof(p.m_QueueFlags), uiHash);
    uiHash = xiiHashingUtils::xxHash64String(p.m_sName.GetView(), uiHash);
    uiHash = xiiHashingUtils::xxHash64(&p.m_bHasSideEffects, sizeof(bool), uiHash);
    for (const ResourceUsage& r : p.m_Reads)
      uiHash = xiiHashingUtils::xxHash64(&r, sizeof(r), uiHash);
    for (const ResourceUsage& w : p.m_Writes)
      uiHash = xiiHashingUtils::xxHash64(&w, sizeof(w), uiHash);
  }
  return uiHash;
}

void xiiRenderGraph::PhaseG_SignatureAndCache(const xiiRGCompileSettings& settings)
{
  const xiiUInt64 uiSig           = ComputeSignature(m_Passes) ^ static_cast<xiiUInt64>(settings.m_uiCacheSalt);
  m_Statistics.m_uiGraphSignature = uiSig;

  if (settings.m_bEnableCompileCache && uiSig == m_uiLastSignature && m_bIsCompiled)
  {
    m_Statistics.m_bUsedCachedCompile = true;
  }
  else
  {
    m_uiLastSignature                 = uiSig;
    m_Statistics.m_bUsedCachedCompile = false;
  }
}

// ============================================================================
//  Execute
// ============================================================================

xiiResult xiiRenderGraph::Execute(
  xiiGALDevice*                pDevice,
  const xiiView*               pView,
  xiiRenderGraphBlackboard*    pBlackboard,
  xiiRenderGraphResourceCache* pResourceCache,
  xiiRenderGraphProfiler*      pProfiler,
  xiiStringBuilder*            out_pError)
{
  XII_ASSERT_DEV(pDevice != nullptr, "Device must not be null.");
  XII_ASSERT_DEV(pBlackboard != nullptr, "Blackboard must not be null.");
  XII_ASSERT_DEV(pResourceCache != nullptr, "ResourceCache must not be null.");

  if (!m_bIsCompiled)
  {
    if (out_pError) *out_pError = "Execute called on an uncompiled render graph.";
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
    ResourceEntry& res = m_Resources[i];
    if (res.m_bIsImported)
    {
      if (res.m_bIsTexture) resolvedTextures[i] = res.m_pImportedTexture;
      else
        resolvedBuffers[i] = res.m_pImportedBuffer;
    }
  }

  // Execute per queue submission.
  for (const xiiRGQueueSubmission& submission : m_QueueSubmissions)
  {
    xiiGALCommandQueue* pQueue = pDevice->GetCommandQueue(submission.m_QueueFlags);
    if (pQueue == nullptr)
    {
      // Fallback: graphics queue handles all work if async queues are unavailable.
      pQueue = pDevice->GetCommandQueue(xiiGALCommandQueueFlags::Graphics);
    }
    XII_ASSERT_DEV(pQueue != nullptr, "Could not obtain a command queue.");

    // Create a command list for this submission.
    xiiGALCommandListCreationDescription clDesc;
    clDesc.m_QueueFlags                          = submission.m_QueueFlags;
    xiiSharedPtr<xiiGALCommandList> pCommandList = pDevice->CreateCommandList(clDesc);
    XII_ASSERT_ALWAYS(pCommandList != nullptr, "Failed to create command list.");

    pCommandList->Begin();

    // Emit cross-queue waits.
    for (xiiUInt32 wi = 0U; wi < submission.m_WaitFences.GetCount(); ++wi)
      pCommandList->DeviceWaitForFence(submission.m_WaitFences[wi], submission.m_WaitValues[wi]);

    xiiUInt32 uiCurrentMergeGroup = xiiInvalidIndex;
    bool      bInsideRenderPass   = false;

    for (xiiUInt32 sortedPos : submission.m_PassOrder)
    {
      xiiRGCompiledPass& cp = m_CompiledPasses[sortedPos];
      if (cp.m_bIsCulled) continue;

      // Acquire transient resources whose lifetime starts at this pass.
      for (xiiUInt32 resIdx : cp.m_AcquireResourceIndices)
      {
        ResourceEntry& res = m_Resources[resIdx];
        if (res.m_bIsTexture)
          resolvedTextures[resIdx] = pResourceCache->AcquireTexture(res.m_TextureDesc);
        else
          resolvedBuffers[resIdx] = pResourceCache->AcquireBuffer(res.m_BufferDesc);
      }

      // Emit pre-barriers (split-bar ends + immediate barriers).
      if (!cp.m_PreBarrierIndices.IsEmpty())
      {
        xiiSmallArray<xiiGALStateTransitionDescription, 8> transitions;
        for (xiiUInt32 bIdx : cp.m_PreBarrierIndices)
        {
          const xiiRGBarrierDesc&           b  = m_Barriers[bIdx];
          xiiGALStateTransitionDescription& td = transitions.ExpandAndGetRef();
          if (b.m_bIsTexture)
            td.m_pResource = resolvedTextures[b.m_uiResourceIndex];
          else
            td.m_pResource = resolvedBuffers[b.m_uiResourceIndex];
          td.m_OldState          = b.m_BeforeState;
          td.m_NewState          = b.m_AfterState;
          td.m_TransitionType    = b.m_TransitionType;
          td.m_TransitionFlags   = b.m_TransitionFlags;
          td.m_uiFirstMipLevel   = b.m_uiFirstMipLevel;
          td.m_uiMipLevelCount   = b.m_uiMipLevelCount;
          td.m_uiFirstArraySlice = b.m_uiFirstArraySlice;
          td.m_uiArraySliceCount = b.m_uiArraySliceCount;
        }
        pCommandList->TransitionResourceStates(xiiMakeArrayPtr(transitions.GetData(), transitions.GetCount()));
      }

      // Handle merge group open.
      if (cp.m_uiMergeGroupIndex != xiiInvalidIndex && cp.m_uiMergeGroupIndex != uiCurrentMergeGroup)
      {
        xiiRGMergeGroup& group = m_MergeGroups[cp.m_uiMergeGroupIndex];
        if (group.m_pNativeRenderPass != nullptr && group.m_pFramebuffer != nullptr)
        {
          xiiGALBeginRenderPassDescription rpDesc;
          rpDesc.m_pRenderPass  = group.m_pNativeRenderPass;
          rpDesc.m_pFramebuffer = group.m_pFramebuffer;
          pCommandList->BeginRenderPass(rpDesc);
          bInsideRenderPass = true;
        }
        uiCurrentMergeGroup = cp.m_uiMergeGroupIndex;
      }
      else if (cp.m_uiMergeGroupIndex == xiiInvalidIndex && bInsideRenderPass)
      {
        pCommandList->EndRenderPass();
        bInsideRenderPass   = false;
        uiCurrentMergeGroup = xiiInvalidIndex;
      }

      // Debug group.
      {
        XII_COMMANDLIST_SCOPE_COLOR(pCommandList, cp.m_sName.GetView(), xiiColor::White);
      }

      // Profiler begin.
      if (pProfiler && m_LastCompileSettings.m_bEnableGPUProfiling)
        pProfiler->OnPassBegin(*pCommandList, cp.m_sName, cp.m_uiPassIndex);

      // Execute pass.
      xiiRGPassContext ctx;
      ctx.m_pCommandList     = pCommandList.Borrow();
      ctx.m_pBlackboard      = pBlackboard;
      ctx.m_pResourceCache   = pResourceCache;
      ctx.m_pView            = pView;
      ctx.m_uiFrameIndex     = m_uiFrameIndex;
      ctx.m_sPassName        = cp.m_sName;
      ctx.m_ResolvedTextures = xiiMakeArrayPtr(resolvedTextures.GetData(), resolvedTextures.GetCount());
      ctx.m_ResolvedBuffers  = xiiMakeArrayPtr(resolvedBuffers.GetData(), resolvedBuffers.GetCount());

      cp.m_ExecuteDelegate(ctx);

      // Profiler end.
      if (pProfiler && m_LastCompileSettings.m_bEnableGPUProfiling)
        pProfiler->OnPassEnd(*pCommandList, cp.m_sName, cp.m_uiPassIndex);

      // Close merge group if last pass in group.
      if (bInsideRenderPass && cp.m_uiMergeGroupIndex != xiiInvalidIndex)
      {
        const xiiRGMergeGroup& group = m_MergeGroups[cp.m_uiMergeGroupIndex];
        if (!group.m_PassIndices.IsEmpty() && group.m_PassIndices.PeekBack() == sortedPos)
        {
          pCommandList->EndRenderPass();
          bInsideRenderPass   = false;
          uiCurrentMergeGroup = xiiInvalidIndex;
        }
      }

      // Emit split-barrier begins (post-pass).
      if (!cp.m_PostBarrierBeginIndices.IsEmpty())
      {
        xiiSmallArray<xiiGALStateTransitionDescription, 8> transitions;
        for (xiiUInt32 bIdx : cp.m_PostBarrierBeginIndices)
        {
          const xiiRGBarrierDesc&           b  = m_Barriers[bIdx];
          xiiGALStateTransitionDescription& td = transitions.ExpandAndGetRef();
          if (b.m_bIsTexture)
            td.m_pResource = resolvedTextures[b.m_uiResourceIndex];
          else
            td.m_pResource = resolvedBuffers[b.m_uiResourceIndex];
          td.m_OldState        = b.m_BeforeState;
          td.m_NewState        = b.m_AfterState;
          td.m_TransitionType  = xiiGALStateTransitionType::Begin;
          td.m_TransitionFlags = b.m_TransitionFlags;
        }
        pCommandList->TransitionResourceStates(xiiMakeArrayPtr(transitions.GetData(), transitions.GetCount()));
      }

      // Release transient resources whose lifetime ends at this pass.
      for (xiiUInt32 resIdx : cp.m_ReleaseResourceIndices)
      {
        ResourceEntry& res = m_Resources[resIdx];
        if (res.m_bIsTexture && resolvedTextures[resIdx])
        {
          pResourceCache->ReturnTexture(resolvedTextures[resIdx]);
          resolvedTextures[resIdx] = nullptr;
        }
        else if (!res.m_bIsTexture && resolvedBuffers[resIdx])
        {
          pResourceCache->ReturnBuffer(resolvedBuffers[resIdx]);
          resolvedBuffers[resIdx] = nullptr;
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
      pCommandList->EnqueueSignal(submission.m_pSignalFence, submission.m_uiSignalValue);

    pCommandList->End();
    pQueue->Submit(pCommandList);
  } // per-submission loop

  // Notify profiler.
  if (pProfiler)
    pProfiler->OnFrameEnd(m_uiFrameIndex);

  return XII_SUCCESS;
}

// ============================================================================
//  DumpToDot
// ============================================================================

xiiResult xiiRenderGraph::DumpToDot(xiiStringBuilder& out_sDot) const
{
  if (!m_bIsCompiled)
    return XII_FAILURE;

  return xiiRenderGraphDebug::DumpToDot(
    xiiMakeArrayPtr(m_CompiledPasses.GetData(), m_CompiledPasses.GetCount()),
    xiiMakeArrayPtr(m_Barriers.GetData(), m_Barriers.GetCount()),
    xiiMakeArrayPtr(m_MergeGroups.GetData(), m_MergeGroups.GetCount()),
    xiiMakeArrayPtr(m_QueueSubmissions.GetData(), m_QueueSubmissions.GetCount()),
    out_sDot);
}
