/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, PipelineCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiGALPipelineCache);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGALPipelineCache* pSingleton = xiiGALPipelineCache::GetSingleton();

    XII_DEFAULT_DELETE(pSingleton);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_IMPLEMENT_SINGLETON(xiiGALPipelineCache);

xiiGALPipelineCache::xiiGALPipelineCache() :
  m_SingletonRegistrar(this), m_pDevice(xiiGALDevice::GetDefaultDevice())
{
}

xiiGALPipelineCache::~xiiGALPipelineCache() = default;

void xiiGALPipelineCache::Clear()
{
  XII_LOCK(m_Mutex);

  m_GraphicsPipelines.Clear();
  m_ComputePipelines.Clear();
}

xiiSharedPtr<xiiGALGraphicsPipelineState> xiiGALPipelineCache::GetPipeline(const xiiGALGraphicsPipelineStateCreationDescription& description)
{
  xiiGALPipelineCache* pCache = xiiGALPipelineCache::GetSingleton();

  xiiSharedPtr<xiiGALGraphicsPipelineState> pGraphicsPipeline = pCache->TryGetPipeline<xiiSharedPtr<xiiGALGraphicsPipelineState>>(description, pCache->m_GraphicsPipelines);

  if (!pGraphicsPipeline)
  {
    pGraphicsPipeline = pCache->m_pDevice->CreateGraphicsPipelineState(description);

    if (!pGraphicsPipeline)
      return {};

    pCache->TryInsertPipeline<xiiSharedPtr<xiiGALGraphicsPipelineState>>(description, pGraphicsPipeline, pCache->m_GraphicsPipelines).IgnoreResult();
  }

  return pGraphicsPipeline;
}

xiiSharedPtr<xiiGALComputePipelineState> xiiGALPipelineCache::GetPipeline(const xiiGALComputePipelineStateCreationDescription& description)
{
  xiiGALPipelineCache* pCache = xiiGALPipelineCache::GetSingleton();

  xiiSharedPtr<xiiGALComputePipelineState> pComputePipeline = pCache->TryGetPipeline<xiiSharedPtr<xiiGALComputePipelineState>>(description, pCache->m_ComputePipelines);

  if (!pComputePipeline)
  {
    pComputePipeline = pCache->m_pDevice->CreateComputePipelineState(description);

    if (!pComputePipeline)
      return {};

    pCache->TryInsertPipeline<xiiSharedPtr<xiiGALComputePipelineState>>(description, pComputePipeline, pCache->m_ComputePipelines).IgnoreResult();
  }

  return pComputePipeline;
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_GPUResourcePool_Implementation_PipelineStateCache);
