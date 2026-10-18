/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

void xiiRenderGraph::BuildTopologyCacheKey(const xiiRenderGraphCompileSettings& settings, xiiDynamicArray<xiiUInt32>& key) const
{
  key.PushBack(settings.m_bEnablePassCulling);
  key.PushBack(settings.m_uiCacheSalt);
  key.PushBack(m_Passes.GetCount());
  key.PushBack(m_Resources.GetCount());
  for (const auto& pass : m_Passes)
  {
    key.PushBack(pass.m_bHasSideEffects);
    key.PushBack(pass.m_Reads.GetCount());
    for (const auto& read : pass.m_Reads)
    {
      key.PushBack(read.m_uiResourceIndex);
      key.PushBack(read.m_uiVersion);
    }
    key.PushBack(pass.m_Writes.GetCount());
    for (const auto& write : pass.m_Writes)
    {
      key.PushBack(write.m_uiResourceIndex);
      key.PushBack(write.m_uiVersion);
    }
  }
  for (const auto& resource : m_Resources)
  {
    key.PushBack(resource.m_Versions.GetCount());
    for (const auto& version : resource.m_Versions)
    {
      key.PushBack(version.m_uiParentVersion);
      key.PushBack(version.m_uiProducerPassIdx);
      key.PushBack(version.m_bExported);
    }
  }
}

bool xiiRenderGraph::RestoreTopologyCache(const xiiRenderGraphCompileSettings& settings, xiiDynamicArray<xiiUInt32>& out_order)
{
  if (!settings.m_bEnableCompileCache || !m_bTopologyCacheValid)
    return false;
  xiiDynamicArray<xiiUInt32> key;
  BuildTopologyCacheKey(settings, key);
  // Compare structural records, not only a hash. Callbacks, imported objects and GAL state are
  // deliberately absent. Resource states, lifetimes and allocation descriptions are re-evaluated.
  if (key != m_CachedTopologyKey)
    return false;
  out_order = m_CachedTopologyOrder;
  for (xiiUInt32 i = 0; i < out_order.GetCount(); ++i)
  {
    const auto  index             = out_order[i];
    const auto& pass              = m_Passes[index];
    auto&       compiled          = m_CompiledPasses.ExpandAndGetRef();
    compiled.m_Id                 = pass.m_Id;
    compiled.m_sName              = pass.m_sName;
    compiled.m_uiPassIndex        = index;
    compiled.m_bHasSideEffects    = pass.m_bHasSideEffects;
    compiled.m_bAllowMerge        = pass.m_bAllowMerge;
    compiled.m_bManagedRenderPass = pass.m_bManagedRenderPass;
    compiled.m_bIsCulled          = m_CachedTopologyCulled[i] != 0;
    compiled.m_pPassData          = pass.m_pPassData;
    compiled.m_ExecuteDelegate    = pass.m_ExecuteDelegate;
    compiled.m_PrepareDelegate    = pass.m_PrepareDelegate;
    m_Statistics.m_uiCulledPassCount += compiled.m_bIsCulled ? 1 : 0;
  }
  m_Statistics.m_bUsedCachedCompile = true;
  return true;
}

void xiiRenderGraph::StoreTopologyCache(const xiiRenderGraphCompileSettings& settings, xiiArrayPtr<const xiiUInt32> order)
{
  m_CachedTopologyKey.Clear();
  BuildTopologyCacheKey(settings, m_CachedTopologyKey);
  m_CachedTopologyOrder = order;
  m_CachedTopologyCulled.Clear();
  for (const auto& pass : m_CompiledPasses)
    m_CachedTopologyCulled.PushBack(pass.m_bIsCulled ? 1 : 0);
  m_bTopologyCacheValid = true;
}
