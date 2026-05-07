/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/Utilities/DescriptorHash.h>

template <typename HandleType, typename DescriptorType, typename KeyType>
HandleType xiiGALRenderPassCache::TryGetRenderPass(const DescriptorType& description, xiiHashTable<KeyType, HandleType, xiiGALRenderPassCache::CacheKeyHasher>& table)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "GAL device not initialized.");

  KeyType key;
  key.m_Description = description;
  key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

  {
    XII_LOCK(m_Mutex);

    HandleType* pExistingRenderPass = table.GetValue(key);
    if (pExistingRenderPass != nullptr)
    {
      return *pExistingRenderPass;
    }
  }
  return {};
}

template <typename HandleType, typename DescriptorType, typename KeyType>
XII_ALWAYS_INLINE xiiResult xiiGALRenderPassCache::TryInsertRenderPass(const DescriptorType& description, HandleType hNewRenderPass, xiiHashTable<KeyType, HandleType, xiiGALRenderPassCache::CacheKeyHasher>& table)
{
  KeyType key;
  key.m_Description = description;
  key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

  XII_LOCK(m_Mutex);

  HandleType hExistingRenderPass;
  if (table.Insert(key, hNewRenderPass, &hExistingRenderPass))
  {
    XII_ASSERT_DEBUG(hExistingRenderPass == hNewRenderPass, "On collision, both pipelines must be the same (create should have just increased the ref count).");

    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
