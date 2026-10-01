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
  template <typename HandleType, typename DescriptorType, typename KeyType>
  HandleType TryGetPipeline(const DescriptorType& description, xiiHashTable<KeyType, HandleType, CacheKeyHasher>& table)
  {
    XII_ASSERT_DEV(m_pDevice != nullptr, "GAL device not initialized.");

    KeyType key;
    key.m_Description = description;
    key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

    XII_LOCK(m_Mutex);
    if (HandleType* pExistingPipeline = table.GetValue(key))
      return *pExistingPipeline;
    return {};
  }

  template <typename HandleType, typename DescriptorType, typename KeyType>
  xiiResult TryInsertPipeline(const DescriptorType& description, HandleType hNewPipeline, xiiHashTable<KeyType, HandleType, CacheKeyHasher>& table)
  {
    KeyType key;
    key.m_Description = description;
    key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

    XII_LOCK(m_Mutex);
    HandleType hExistingPipeline;
    if (table.Insert(key, hNewPipeline, &hExistingPipeline))
    {
      XII_ASSERT_DEBUG(hExistingPipeline == hNewPipeline, "On collision, both pipelines must be the same (create should have just increased the ref count).");
      return XII_FAILURE;
    }
    return XII_SUCCESS;
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

  xiiSharedPtr<xiiGALGraphicsPipelineState> pGraphicsPipeline = s_pState->TryGetPipeline<xiiSharedPtr<xiiGALGraphicsPipelineState>>(description, s_pState->m_GraphicsPipelines);

  if (!pGraphicsPipeline)
  {
    pGraphicsPipeline = s_pState->m_pDevice->CreateGraphicsPipelineState(description);

    if (!pGraphicsPipeline)
      return {};

    s_pState->TryInsertPipeline<xiiSharedPtr<xiiGALGraphicsPipelineState>>(description, pGraphicsPipeline, s_pState->m_GraphicsPipelines).IgnoreResult();
  }

  return pGraphicsPipeline;
}

xiiSharedPtr<xiiGALComputePipelineState> xiiGALPipelineCache::GetPipeline(const xiiGALComputePipelineStateCreationDescription& description)
{
  XII_ASSERT_DEV(IsInitialized(), "The pipeline cache subsystem is not initialized.");
  if (!IsInitialized())
    return {};

  xiiSharedPtr<xiiGALComputePipelineState> pComputePipeline = s_pState->TryGetPipeline<xiiSharedPtr<xiiGALComputePipelineState>>(description, s_pState->m_ComputePipelines);

  if (!pComputePipeline)
  {
    pComputePipeline = s_pState->m_pDevice->CreateComputePipelineState(description);

    if (!pComputePipeline)
      return {};

    s_pState->TryInsertPipeline<xiiSharedPtr<xiiGALComputePipelineState>>(description, pComputePipeline, s_pState->m_ComputePipelines).IgnoreResult();
  }

  return pComputePipeline;
}

xiiSharedPtr<xiiGALRayTracingPipelineState> xiiGALPipelineCache::GetPipeline(const xiiGALRayTracingPipelineStateCreationDescription& description)
{
  XII_ASSERT_DEV(IsInitialized(), "The pipeline cache subsystem is not initialized.");
  if (!IsInitialized())
    return {};

  xiiSharedPtr<xiiGALRayTracingPipelineState> pRayTracingPipeline = s_pState->TryGetPipeline<xiiSharedPtr<xiiGALRayTracingPipelineState>>(description, s_pState->m_RayTracingPipelines);

  if (!pRayTracingPipeline)
  {
    pRayTracingPipeline = s_pState->m_pDevice->CreateRayTracingPipelineState(description);

    if (!pRayTracingPipeline)
      return {};

    s_pState->TryInsertPipeline<xiiSharedPtr<xiiGALRayTracingPipelineState>>(description, pRayTracingPipeline, s_pState->m_RayTracingPipelines).IgnoreResult();
  }

  return pRayTracingPipeline;
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_GPUResourcePool_Implementation_PipelineStateCache);
