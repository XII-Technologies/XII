/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

class xiiGALPipelineCache::State
{
public:
  template <typename PipelinePtr, typename DescriptorType, typename KeyType, typename CreatePipeline>
  PipelinePtr GetOrCreatePipeline(const DescriptorType& description, xiiHashTable<KeyType, PipelinePtr, CacheKeyHasher>& table, CreatePipeline create)
  {
    XII_ASSERT_DEV(m_pDevice != nullptr, "GAL device not initialized.");

    KeyType key;
    key.m_Description = description;
    key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

    XII_LOCK(m_Mutex);
    if (PipelinePtr* pExistingPipeline = table.GetValue(key))
      return *pExistingPipeline;

    // The device creates distinct objects for identical descriptors. Keep a cache miss
    // locked through creation so concurrent worlds share one pipeline and one compilation.
    PipelinePtr pipeline = create();
    if (pipeline)
      table.Insert(key, pipeline);
    return pipeline;
  }

  void Clear()
  {
    XII_LOCK(m_Mutex);
    m_GraphicsPipelines.Clear();
    m_ComputePipelines.Clear();
    m_RayTracingPipelines.Clear();
  }

  xiiMutex                                                                                              m_Mutex;
  xiiSharedPtr<xiiGALDevice>                                                                            m_pDevice;
  xiiHashTable<GraphicsPipelineCacheKey, xiiSharedPtr<xiiGALGraphicsPipelineState>, CacheKeyHasher>     m_GraphicsPipelines;
  xiiHashTable<ComputePipelineCacheKey, xiiSharedPtr<xiiGALComputePipelineState>, CacheKeyHasher>       m_ComputePipelines;
  xiiHashTable<RayTracingPipelineCacheKey, xiiSharedPtr<xiiGALRayTracingPipelineState>, CacheKeyHasher> m_RayTracingPipelines;
};

xiiUniquePtr<xiiGALPipelineCache::State> xiiGALPipelineCache::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, PipelineCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGALPipelineCache::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGALPipelineCache::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGALPipelineCache::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGALPipelineCache::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool xiiGALPipelineCache::IsSubsystemInitialized()
{
  return s_pState != nullptr;
}

bool xiiGALPipelineCache::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_pDevice != nullptr;
}

void xiiGALPipelineCache::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Pipeline cache started twice.");
  s_pState = XII_DEFAULT_NEW(State);
}

void xiiGALPipelineCache::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede pipeline cache engine startup.");
  if (s_pState != nullptr)
    s_pState->m_pDevice = xiiGALDevice::GetDefaultDevice();
}

void xiiGALPipelineCache::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->Clear();
  s_pState->m_pDevice.Clear();
}

void xiiGALPipelineCache::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiSharedPtr<xiiGALGraphicsPipelineState> xiiGALPipelineCache::GetPipeline(const xiiGALGraphicsPipelineStateCreationDescription& description)
{
  XII_ASSERT_DEV(IsInitialized(), "The pipeline cache subsystem is not initialized.");
  if (!IsInitialized())
    return {};

  return s_pState->GetOrCreatePipeline(description, s_pState->m_GraphicsPipelines, [&] { return s_pState->m_pDevice->CreateGraphicsPipelineState(description); });
}

xiiSharedPtr<xiiGALComputePipelineState> xiiGALPipelineCache::GetPipeline(const xiiGALComputePipelineStateCreationDescription& description)
{
  XII_ASSERT_DEV(IsInitialized(), "The pipeline cache subsystem is not initialized.");
  if (!IsInitialized())
    return {};

  return s_pState->GetOrCreatePipeline(description, s_pState->m_ComputePipelines, [&] { return s_pState->m_pDevice->CreateComputePipelineState(description); });
}

xiiSharedPtr<xiiGALRayTracingPipelineState> xiiGALPipelineCache::GetPipeline(const xiiGALRayTracingPipelineStateCreationDescription& description)
{
  XII_ASSERT_DEV(IsInitialized(), "The pipeline cache subsystem is not initialized.");
  if (!IsInitialized())
    return {};

  return s_pState->GetOrCreatePipeline(description, s_pState->m_RayTracingPipelines, [&] { return s_pState->m_pDevice->CreateRayTracingPipelineState(description); });
}

xiiUInt32 xiiGALPipelineCache::CacheKeyHasher::Hash(const xiiGALPipelineCache::GraphicsPipelineCacheKey& a)
{
  return a.m_uiHash;
}

bool xiiGALPipelineCache::CacheKeyHasher::Equal(const xiiGALPipelineCache::GraphicsPipelineCacheKey& a, const xiiGALPipelineCache::GraphicsPipelineCacheKey& b)
{
  return a.m_uiHash == b.m_uiHash && a.m_Description == b.m_Description;
}

xiiUInt32 xiiGALPipelineCache::CacheKeyHasher::Hash(const xiiGALPipelineCache::ComputePipelineCacheKey& a)
{
  return a.m_uiHash;
}

bool xiiGALPipelineCache::CacheKeyHasher::Equal(const xiiGALPipelineCache::ComputePipelineCacheKey& a, const xiiGALPipelineCache::ComputePipelineCacheKey& b)
{
  return a.m_uiHash == b.m_uiHash && a.m_Description == b.m_Description;
}

xiiUInt32 xiiGALPipelineCache::CacheKeyHasher::Hash(const xiiGALPipelineCache::RayTracingPipelineCacheKey& a)
{
  return a.m_uiHash;
}

bool xiiGALPipelineCache::CacheKeyHasher::Equal(const xiiGALPipelineCache::RayTracingPipelineCacheKey& a, const xiiGALPipelineCache::RayTracingPipelineCacheKey& b)
{
  return a.m_uiHash == b.m_uiHash && a.m_Description == b.m_Description;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_PipelineStateCache);
