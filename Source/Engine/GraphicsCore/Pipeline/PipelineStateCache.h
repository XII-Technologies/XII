/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/States/PipelineState.h>

/// A cache from pipeline descriptor to handle which holds a reference to each pipeline that is never freed until shutdown.
class XII_GRAPHICSCORE_DLL xiiGALPipelineCache
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGALPipelineCache);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, PipelineCache);

public:
  xiiGALPipelineCache() = delete;

  [[nodiscard]] static bool IsSubsystemInitialized();
  [[nodiscard]] static bool IsInitialized();

  /// Creates a pipeline or retrieves it from the cache.
  static xiiSharedPtr<xiiGALGraphicsPipelineState> GetPipeline(const xiiGALGraphicsPipelineStateCreationDescription& description);

  /// Creates a pipeline or retrieves it from the cache.
  static xiiSharedPtr<xiiGALComputePipelineState> GetPipeline(const xiiGALComputePipelineStateCreationDescription& description);

  /// Creates a ray-tracing pipeline or retrieves it from the cache.
  static xiiSharedPtr<xiiGALRayTracingPipelineState> GetPipeline(const xiiGALRayTracingPipelineStateCreationDescription& description);

private:
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

  struct RayTracingPipelineCacheKey
  {
    xiiUInt32                                        m_uiHash = 0U;
    xiiGALRayTracingPipelineStateCreationDescription m_Description;
  };

  struct CacheKeyHasher
  {
    static xiiUInt32 Hash(const GraphicsPipelineCacheKey& a);
    static bool      Equal(const GraphicsPipelineCacheKey& a, const GraphicsPipelineCacheKey& b);

    static xiiUInt32 Hash(const ComputePipelineCacheKey& a);
    static bool      Equal(const ComputePipelineCacheKey& a, const ComputePipelineCacheKey& b);

    static xiiUInt32 Hash(const RayTracingPipelineCacheKey& a);
    static bool      Equal(const RayTracingPipelineCacheKey& a, const RayTracingPipelineCacheKey& b);
  };

private:
  class State;

  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  static xiiUniquePtr<State> s_pState;
};
