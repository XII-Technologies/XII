#include <GraphicsFoundation/Utilities/DescriptorHash.h>

template <typename HandleType, typename DescType, typename KeyType>
HandleType xiiGALPipelineCache::TryGetPipeline(const DescType& description, xiiHashTable<KeyType, HandleType, xiiGALPipelineCache::CacheKeyHasher>& table)
{
  XII_ASSERT_DEV(m_pDevice != nullptr, "GAL device not initialized.");

  KeyType key;
  key.m_Description = description;
  key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

  {
    XII_LOCK(m_Mutex);

    HandleType* pExistingPipeline = table.GetValue(key);
    if (pExistingPipeline != nullptr)
    {
      return *pExistingPipeline;
    }
  }
  return {};
}

template <typename HandleType, typename DescType, typename KeyType>
XII_ALWAYS_INLINE xiiResult xiiGALPipelineCache::TryInsertPipeline(const DescType& description, HandleType hNewPipeline, xiiHashTable<KeyType, HandleType, xiiGALPipelineCache::CacheKeyHasher>& table)
{
  KeyType key;
  key.m_Description = description;
  key.m_uiHash      = xiiGALDescriptorHash::Hash(description);

  XII_LOCK(m_Mutex);

  HandleType existingPipeline;
  if (table.Insert(key, hNewPipeline, &existingPipeline))
  {
    XII_ASSERT_DEBUG(existingPipeline == hNewPipeline, "On collision, both pipelines must be the same (create should have just increased the ref count).");

    return XII_FAILURE;
  }
  return XII_SUCCESS;
}
