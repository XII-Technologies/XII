/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Material/MaterialTypes.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>

class xiiGpuInstanceData;
class xiiRayTracingFrameResourcesState;
/// World-owned rotating TLAS allocations. Contains no object handles, object registry or
/// authored transforms; each build consumes the world's extracted draw records.
class XII_GRAPHICSCORE_DLL xiiRayTracingFrameResources
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRayTracingFrameResources);

public:
  xiiRayTracingFrameResources();
  ~xiiRayTracingFrameResources();

private:
  friend class xiiRayTracingResources;
  xiiUniquePtr<xiiRayTracingFrameResourcesState> m_pState;
};

/// Generation-checked lease for one device-resident BLAS geometry.
using xiiRayTracingGeometryId = xiiGenericId<24, 8>;

class XII_GRAPHICSCORE_DLL xiiRayTracingGeometryHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiRayTracingGeometryHandle, xiiRayTracingGeometryId);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingGeometryHandle);

/// Immutable source geometry used to create and cache one BLAS.
struct XII_GRAPHICSCORE_DLL xiiRayTracingGeometryDescription
{
  [[nodiscard]] xiiString GetMeshBufferResourceId() const;
  void                    SetMeshBufferResourceId(xiiString sResourceId);

  xiiMeshBufferResourceHandle m_hMeshBuffer;
  bool                        m_bOpaque          = true;
  xiiUInt32                   m_uiFirstPrimitive = 0;
  xiiUInt32                   m_uiPrimitiveCount = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingGeometryDescription);

/// Device geometry capacity and per-extraction acceleration-structure limits.
struct XII_GRAPHICSCORE_DLL xiiRayTracingResourceLimits
{
  xiiUInt32 m_uiMaxGeometries  = 16384U;
  xiiUInt32 m_uiMaxInstances   = 65536U;
  xiiUInt32 m_uiFramesInFlight = 3U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingResourceLimits);

/// Tool-facing device geometry and acceleration-structure counters.
struct XII_GRAPHICSCORE_DLL xiiRayTracingResourceStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiGeometryCount    = 0U;
  xiiUInt32 m_uiPendingBLASCount = 0U;
  xiiUInt64 m_uiResourceRevision = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingResourceStats);

/// Device-level shared BLAS residency. Owns no scene objects or TLAS instance registry.
///
/// CPU containers are allocated during core-system startup and released during core-system
/// shutdown. Backend resources are created only after the GAL device exists and are released
/// during high-level shutdown. Callers never construct or own a manager object.
class XII_GRAPHICSCORE_DLL xiiRayTracingResources
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRayTracingResources);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RayTracingResources);

public:
  xiiRayTracingResources() = delete;

  friend class xiiRayTracingFrameResourcesState;

  [[nodiscard]] static xiiResult                          Configure(const xiiRayTracingResourceLimits& description);
  [[nodiscard]] static const xiiRayTracingResourceLimits& GetConfiguration();
  [[nodiscard]] static bool                               IsSubsystemInitialized();
  [[nodiscard]] static bool                               IsInitialized();
  [[nodiscard]] static bool                               IsHardwareRayTracingSupported();

  [[nodiscard]] static xiiRayTracingGeometryHandle AcquireGeometry(const xiiRayTracingGeometryDescription& description);
  static void                                      ReleaseGeometry(xiiRayTracingGeometryHandle handle);
  [[nodiscard]] static bool                        IsValid(xiiRayTracingGeometryHandle handle);

  [[nodiscard]] static xiiRayTracingResourceStats GetStats();

  struct BuildHandles
  {
    /// Rotating TLAS owned by the supplied frame resources. Trace-ready after the build pass executes.
    xiiSharedPtr<xiiGALTopLevelAS> m_pTopLevelAS;
    /// Render-graph dependency token for ray-dispatch passes that consume m_pTopLevelAS.
    xiiRenderGraphBufferHandle m_hSceneDependency;
    /// Shader-readable canonical material records indexed by HLSL InstanceIndex().
    xiiRenderGraphBufferHandle m_hMaterialData;
    /// Shader-readable geometry addressing records indexed by HLSL InstanceIndex().
    xiiRenderGraphBufferHandle m_hGeometryData;
    xiiUInt32                  m_uiInstanceCount = 0U;
  };

  /// Adds BLAS/TLAS construction to the graph on the compute queue.
  ///
  /// A frame-in-flight TLAS ring prevents updates from mutating acceleration structures still used
  /// by older GPU submissions. The returned dependency buffer must be declared as a BuildASRead by
  /// every trace pass that consumes the returned TLAS.
  [[nodiscard]] static BuildHandles AddBuildPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiRayTracingFrameResources& worldResources, const xiiGpuInstanceData& instances, const xiiArrayPtr<const xiiUInt32>* selection = nullptr);

private:
  static void      Startup();
  static void      EngineStartup();
  static void      EngineShutdown();
  static void      Shutdown();
  static xiiResult ApplyConfiguration();
  static bool      PrepareGeometry(xiiUInt32 uiGeometryIndex);
  static bool      PrepareFrameResources(xiiRayTracingFrameResources& resources, xiiUInt32 uiFrameSlot, xiiUInt32 uiInstanceCount);

  struct GeometrySlot;
  struct FrameResources;
  class State;
  static xiiUniquePtr<State> s_pState;
};
