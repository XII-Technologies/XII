/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RenderPassCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    XII_DEFAULT_NEW(xiiGALRenderPassCache);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGALRenderPassCache* pSingleton = xiiGALRenderPassCache::GetSingleton();
    XII_DEFAULT_DELETE(pSingleton);
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_IMPLEMENT_SINGLETON(xiiGALRenderPassCache);

xiiGALRenderPassCache::xiiGALRenderPassCache() :
  m_SingletonRegistrar(this), m_pDevice(xiiGALDevice::GetDefaultDevice())
{
}

xiiGALRenderPassCache::~xiiGALRenderPassCache() = default;

void xiiGALRenderPassCache::Clear()
{
  XII_LOCK(m_Mutex);

  m_RenderPasses.Clear();
}

xiiSharedPtr<xiiGALRenderPass> xiiGALRenderPassCache::GetRenderPass(const xiiGALRenderPassCreationDescription& description)
{
  xiiGALRenderPassCache* pCache = xiiGALRenderPassCache::GetSingleton();

  xiiSharedPtr<xiiGALRenderPass> pRenderPass = pCache->TryGetRenderPass<xiiSharedPtr<xiiGALRenderPass>>(description, pCache->m_RenderPasses);

  if (!pRenderPass)
  {
    pRenderPass = pCache->m_pDevice->CreateRenderPass(description);

    if (!pRenderPass)
      return {};

    pCache->TryInsertRenderPass<xiiSharedPtr<xiiGALRenderPass>>(description, pRenderPass, pCache->m_RenderPasses).IgnoreResult();
  }

  return pRenderPass;
}

xiiUInt32 xiiGALRenderPassCache::CacheKeyHasher::Hash(const xiiGALRenderPassCache::RenderPassCacheKey& a)
{
  return a.m_uiHash;
}

bool xiiGALRenderPassCache::CacheKeyHasher::Equal(const xiiGALRenderPassCache::RenderPassCacheKey& a, const xiiGALRenderPassCache::RenderPassCacheKey& b)
{
  return a.m_uiHash == b.m_uiHash && a.m_Description == b.m_Description;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_GPUResourcePool_Implementation_RenderPassCache);
