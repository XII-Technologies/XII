/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Geometry/GeometryResidency.h>
#include <GraphicsCore/Material/MaterialGpuStorage.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Visibility/GpuVisibilitySystem.h>
#include <Shaders/Pipeline/Passes/ReSTIR/EmissiveLightData.h>

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuEmissiveTriangle);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuEmissiveLightHeader);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuEmissiveAliasEntry);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuEmissiveAliasHeader);

/// Capacity and quality controls for GPU emissive-triangle extraction.
struct XII_GRAPHICSCORE_DLL xiiEmissiveLightSettings
{
  xiiUInt32 m_uiMaxTriangles = 262144U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiEmissiveLightSettings);

/// CPU-visible scheduling statistics. Exact emitted/overflow counts remain in the GPU header so
/// tools can inspect them without introducing a synchronous readback into the render loop.
struct XII_GRAPHICSCORE_DLL xiiEmissiveLightStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiTriangleCapacity             = 0U;
  xiiUInt32 m_uiScheduledInstanceCount       = 0U;
  xiiUInt32 m_uiScheduledMeshletsPerInstance = 0U;
  xiiUInt64 m_uiLastScheduledFrame           = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiEmissiveLightStats);

/// Process-wide GPU emissive-light extraction service.
///
/// The service does not retain CPU mesh copies or duplicate scene registrations. Each graph build
/// consumes the generation-bearing geometry table, frame-sliced scene instances, and canonical
/// material table, then reconstructs world-space emissive triangles from bindless mesh buffers.
/// Allocator-backed state exists only between GraphicsCore subsystem startup and shutdown.
class XII_GRAPHICSCORE_DLL xiiEmissiveLightManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiEmissiveLightManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, EmissiveLightManager);

public:
  xiiEmissiveLightManager() = delete;

  [[nodiscard]] static xiiResult                       Configure(const xiiEmissiveLightSettings& settings);
  [[nodiscard]] static const xiiEmissiveLightSettings& GetConfiguration();
  [[nodiscard]] static xiiEmissiveLightStats           GetStats();
  [[nodiscard]] static bool                            IsSubsystemInitialized();
  [[nodiscard]] static bool                            IsInitialized();

  struct BuildDescription
  {
    xiiGpuVisibilityOutputs                    m_Visibility;
    xiiGeometryResidencyManager::UploadHandles m_Geometry;
    xiiMaterialGpuStorage::UploadHandles       m_Materials;
    xiiUInt32                                  m_uiInstanceCount  = 0U;
    xiiUInt32                                  m_uiVertexStride   = 0U;
    xiiUInt32                                  m_uiTexCoordOffset = 0U;
  };

  struct BuildOutputs
  {
    xiiRenderGraphBufferHandle m_hTriangles;
    xiiRenderGraphBufferHandle m_hHeader;
    xiiRenderGraphBufferHandle m_hTriangleAliases;
    xiiRenderGraphBufferHandle m_hBlockAliases;
    xiiRenderGraphBufferHandle m_hAliasHeader;
    xiiUInt32                  m_uiTriangleCapacity = 0U;

    [[nodiscard]] bool IsValid() const
    {
      return m_hTriangles.IsValid() && m_hHeader.IsValid() && m_hTriangleAliases.IsValid() && m_hBlockAliases.IsValid() && m_hAliasHeader.IsValid();
    }
  };

  /// Adds async-compute extraction and power-weighted distribution passes. Outputs are explicit
  /// render-graph versions and must be passed directly to every consumer; there is intentionally
  /// no mutable "current buffer" API.
  [[nodiscard]] static BuildOutputs AddBuildPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, const BuildDescription& description);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();

  class State;
  static xiiUniquePtr<State> s_pState;
};
