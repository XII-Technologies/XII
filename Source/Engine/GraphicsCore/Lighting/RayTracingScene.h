/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsFoundation/Resources/TopLevelAS.h>

/// Generation-checked handle for one mesh geometry in the hardware ray-tracing scene.
struct XII_GRAPHICSCORE_DLL xiiRayTracingGeometryHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingGeometryHandle);

/// Generation-checked handle for one transformable TLAS instance.
struct XII_GRAPHICSCORE_DLL xiiRayTracingInstanceHandle
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }

  xiiUInt32 m_uiIndex      = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingInstanceHandle);

/// Immutable source geometry used to create and cache one BLAS.
struct XII_GRAPHICSCORE_DLL xiiRayTracingGeometryDescription
{
  [[nodiscard]] xiiString GetMeshBufferResourceId() const;
  void                    SetMeshBufferResourceId(xiiString sResourceId);

  xiiMeshBufferResourceHandle m_hMeshBuffer;
  bool                        m_bOpaque = true;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingGeometryDescription);

/// Mutable scene instance. Transform updates only invalidate the rotating TLAS frame slice.
struct XII_GRAPHICSCORE_DLL xiiRayTracingInstanceDescription
{
  xiiRayTracingGeometryHandle               m_hGeometry;
  xiiMat4                                   m_Transform       = xiiMat4::MakeIdentity();
  xiiUInt32                                 m_uiStableObjectId = 0U;
  xiiUInt8                                  m_uiVisibilityMask = 0xFFU;
  xiiBitflags<xiiGALRayTracingInstanceFlags> m_Flags;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingInstanceDescription);

/// Process startup limits for the hardware ray-tracing scene.
struct XII_GRAPHICSCORE_DLL xiiRayTracingSceneDescription
{
  xiiUInt32 m_uiMaxGeometries  = 16384U;
  xiiUInt32 m_uiMaxInstances   = 65536U;
  xiiUInt32 m_uiFramesInFlight = 3U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingSceneDescription);

/// Tool-facing scene and acceleration-structure counters.
struct XII_GRAPHICSCORE_DLL xiiRayTracingSceneStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiGeometryCount    = 0U;
  xiiUInt32 m_uiInstanceCount    = 0U;
  xiiUInt32 m_uiPendingBLASCount = 0U;
  xiiUInt64 m_uiSceneRevision    = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiRayTracingSceneStats);

/// Process-wide ray-tracing scene service.
///
/// CPU containers are allocated during core-system startup and released during core-system
/// shutdown. Backend resources are created only after the GAL device exists and are released
/// during high-level shutdown. Callers never construct or own a manager object.
class XII_GRAPHICSCORE_DLL xiiRayTracingSceneManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRayTracingSceneManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, RayTracingSceneManager);

public:
  xiiRayTracingSceneManager() = delete;

  [[nodiscard]] static xiiResult                             Configure(const xiiRayTracingSceneDescription& description);
  [[nodiscard]] static const xiiRayTracingSceneDescription& GetConfiguration();
  [[nodiscard]] static bool                                  IsInitialized();
  [[nodiscard]] static bool                                  IsHardwareRayTracingSupported();

  [[nodiscard]] static xiiRayTracingGeometryHandle RegisterGeometry(const xiiRayTracingGeometryDescription& description);
  static void                                                UnregisterGeometry(xiiRayTracingGeometryHandle handle);
  [[nodiscard]] static bool                                  IsValid(xiiRayTracingGeometryHandle handle);

  [[nodiscard]] static xiiRayTracingInstanceHandle CreateInstance(const xiiRayTracingInstanceDescription& description);
  static void                                                DestroyInstance(xiiRayTracingInstanceHandle handle);
  [[nodiscard]] static bool                                  UpdateInstance(xiiRayTracingInstanceHandle handle, const xiiRayTracingInstanceDescription& description);
  [[nodiscard]] static bool                                  IsValid(xiiRayTracingInstanceHandle handle);

  [[nodiscard]] static xiiRayTracingSceneStats GetStats();

private:
  static void      Startup();
  static void      EngineStartup();
  static void      EngineShutdown();
  static void      Shutdown();
  static xiiResult ApplyConfiguration();

  struct GeometrySlot;
  struct InstanceSlot;
  class State;
  static xiiUniquePtr<State> s_pState;
};

