#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Threading/Mutex.h>

#include <GraphicsFoundation/States/PipelineState.h>

class xiiGALDevice;

/// \brief A cache from pipeline descriptor to handle which holds a reference to each pipeline that is never freed until shutdown.
class XII_GRAPHICSCORE_DLL xiiGALPipelineCache
{
  XII_DECLARE_SINGLETON(xiiGALPipelineCache);

public:
  /// \brief Creates a pipeline or retrieves it from the cache.
  static xiiSharedPtr<xiiGALGraphicsPipelineState> GetPipeline(const xiiGALGraphicsPipelineStateCreationDescription& description);

  /// \brief Creates a pipeline or retrieves it from the cache.
  static xiiSharedPtr<xiiGALComputePipelineState> GetPipeline(const xiiGALComputePipelineStateCreationDescription& description);

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, PipelineCache);

  friend class xiiMemoryUtils;

  struct GraphicsPipelineCacheKey
  {
    xiiUInt32                                      m_uiHash = 0U;
    xiiGALGraphicsPipelineStateCreationDescription m_Description;
  };

  struct ComputePipelineCacheKey
  {
    xiiUInt32                                     m_uiHash = 0U;
    xiiGALComputePipelineStateCreationDescription m_Description;
  };

  struct CacheKeyHasher
  {
    static xiiUInt32 Hash(const GraphicsPipelineCacheKey& a);
    static bool      Equal(const GraphicsPipelineCacheKey& a, const GraphicsPipelineCacheKey& b);

    static xiiUInt32 Hash(const ComputePipelineCacheKey& a);
    static bool      Equal(const ComputePipelineCacheKey& a, const ComputePipelineCacheKey& b);
  };

private:
  xiiGALPipelineCache();
  ~xiiGALPipelineCache();

  void GALDeviceEventHandler(const xiiGALDeviceEvent& e);

  void Clear();

  template <typename HandleType, typename DescType, typename KeyType>
  XII_ALWAYS_INLINE HandleType TryGetPipeline(const DescType& description, xiiHashTable<KeyType, HandleType, CacheKeyHasher>& table);

  template <typename HandleType, typename DescType, typename KeyType>
  XII_ALWAYS_INLINE xiiResult TryInsertPipeline(const DescType& description, HandleType hNewPipeline, xiiHashTable<KeyType, HandleType, CacheKeyHasher>& table);

private:
  xiiMutex                                                                                          m_Mutex;
  xiiGALDevice*                                                                                     m_pDevice;
  xiiHashTable<GraphicsPipelineCacheKey, xiiSharedPtr<xiiGALGraphicsPipelineState>, CacheKeyHasher> m_GraphicsPipelines;
  xiiHashTable<ComputePipelineCacheKey, xiiSharedPtr<xiiGALComputePipelineState>, CacheKeyHasher>   m_ComputePipelines;
};

#include <GraphicsCore/GPUResourcePool/Implementation/PipelineStateCache_inl.h>
