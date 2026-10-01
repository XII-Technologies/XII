/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Pipeline/RenderPassCache.h>
#include <GraphicsFoundation/Utilities/DescriptorHash.h>

class xiiGALRenderPassCache::State
{
public:
  template <typename HandleType, typename DescriptorType, typename KeyType>
  HandleType TryGetRenderPass(const DescriptorType& description, xiiHashTable<KeyType, HandleType, CacheKeyHasher>& table)
  {
    XII_ASSERT_DEV(m_pDevice != nullptr, "GAL device not initialized.");

    KeyType key;
    key.m_Description = description;
    key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

    XII_LOCK(m_Mutex);
    if (HandleType* pExistingRenderPass = table.GetValue(key))
      return *pExistingRenderPass;
    return {};
  }

  template <typename HandleType, typename DescriptorType, typename KeyType>
  xiiResult TryInsertRenderPass(const DescriptorType& description, HandleType hNewRenderPass, xiiHashTable<KeyType, HandleType, CacheKeyHasher>& table)
  {
    KeyType key;
    key.m_Description = description;
    key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

    XII_LOCK(m_Mutex);
    HandleType hExistingRenderPass;
    if (table.Insert(key, hNewRenderPass, &hExistingRenderPass))
    {
      XII_ASSERT_DEBUG(hExistingRenderPass == hNewRenderPass, "On collision, both render passes must be the same (create should have just increased the ref count).");
      return XII_FAILURE;
    }
    return XII_SUCCESS;
  }

  void Clear()
  {
    XII_LOCK(m_Mutex);
    m_RenderPasses.Clear();
  }

  xiiMutex                                                                         m_Mutex;
  xiiSharedPtr<xiiGALDevice>                                                       m_pDevice;
  xiiHashTable<RenderPassCacheKey, xiiSharedPtr<xiiGALRenderPass>, CacheKeyHasher> m_RenderPasses;
};

xiiUniquePtr<xiiGALRenderPassCache::State> xiiGALRenderPassCache::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, RenderPassCache)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiGALRenderPassCache::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiGALRenderPassCache::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiGALRenderPassCache::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiGALRenderPassCache::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool xiiGALRenderPassCache::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_pDevice != nullptr;
}

void xiiGALRenderPassCache::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Render-pass cache started twice.");
  s_pState = XII_DEFAULT_NEW(State);
}

void xiiGALRenderPassCache::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede render-pass cache engine startup.");
  if (s_pState != nullptr)
    s_pState->m_pDevice = xiiGALDevice::GetDefaultDevice();
}

void xiiGALRenderPassCache::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  s_pState->Clear();
  s_pState->m_pDevice.Clear();
}

void xiiGALRenderPassCache::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiSharedPtr<xiiGALRenderPass> xiiGALRenderPassCache::GetRenderPass(const xiiGALRenderPassCreationDescription& description)
{
  XII_ASSERT_DEV(IsInitialized(), "The render-pass cache subsystem is not initialized.");
  if (!IsInitialized())
    return {};

  xiiSharedPtr<xiiGALRenderPass> pRenderPass = s_pState->TryGetRenderPass<xiiSharedPtr<xiiGALRenderPass>>(description, s_pState->m_RenderPasses);

  if (!pRenderPass)
  {
    pRenderPass = s_pState->m_pDevice->CreateRenderPass(description);

    if (!pRenderPass)
      return {};

    s_pState->TryInsertRenderPass<xiiSharedPtr<xiiGALRenderPass>>(description, pRenderPass, s_pState->m_RenderPasses).IgnoreResult();
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
