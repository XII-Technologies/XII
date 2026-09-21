/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>

#include <GraphicsFoundation/Resources/RenderPass.h>

/// A cache from pipeline descriptor to handle which holds a reference to each pipeline that is never freed until shutdown.
class XII_GRAPHICSCORE_DLL xiiGALRenderPassCache
{
  XII_DECLARE_SINGLETON(xiiGALRenderPassCache);

public:
  /// Creates a render pass or retrieves it from the cache.
  static xiiSharedPtr<xiiGALRenderPass> GetRenderPass(const xiiGALRenderPassCreationDescription& description);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RenderPassCache);

  friend class xiiMemoryUtils;

  struct RenderPassCacheKey
  {
    xiiUInt32                           m_uiHash = 0U;
    xiiGALRenderPassCreationDescription m_Description;
  };

  struct CacheKeyHasher
  {
    static xiiUInt32 Hash(const RenderPassCacheKey& a);
    static bool      Equal(const RenderPassCacheKey& a, const RenderPassCacheKey& b);
  };

private:
  xiiGALRenderPassCache();
  ~xiiGALRenderPassCache();

  void Clear();

  template <typename HandleType, typename DescriptorType, typename KeyType>
  XII_ALWAYS_INLINE HandleType TryGetRenderPass(const DescriptorType& description, xiiHashTable<KeyType, HandleType, CacheKeyHasher>& table);

  template <typename HandleType, typename DescriptorType, typename KeyType>
  XII_ALWAYS_INLINE xiiResult TryInsertRenderPass(const DescriptorType& description, HandleType hNewRenderPass, xiiHashTable<KeyType, HandleType, CacheKeyHasher>& table);

private:
  xiiMutex                                                                         m_Mutex;
  xiiGALDevice*                                                                    m_pDevice;
  xiiHashTable<RenderPassCacheKey, xiiSharedPtr<xiiGALRenderPass>, CacheKeyHasher> m_RenderPasses;
};

#include <GraphicsCore/Pipeline/Implementation/RenderPassCache_inl.h>
