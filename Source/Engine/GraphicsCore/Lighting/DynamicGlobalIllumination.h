/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

class xiiLightingSystem;

/// Runtime controls for the camera-centred mid-field DDGI clipmap.
struct XII_GRAPHICSCORE_DLL xiiDDGISettings
{
  xiiUInt32 m_uiProbeCountX              = 24U;
  xiiUInt32 m_uiProbeCountY              = 12U;
  xiiUInt32 m_uiProbeCountZ              = 24U;
  float     m_fProbeSpacing              = 4.0f;
  xiiUInt32 m_uiProbeUpdateBudget        = 128U;
  float     m_fTemporalHysteresis        = 0.97f;
  float     m_fMaximumRelocationDistance = 1.8f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGISettings);

struct XII_GRAPHICSCORE_DLL xiiDDGIProbeFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None           = 0U,
    Valid          = XII_BIT(0),
    NeedsUpdate    = XII_BIT(1),
    Relocated      = XII_BIT(2),
    InsideGeometry = XII_BIT(3),

    Default = None
  };

  struct Bits
  {
    StorageType Valid : 1;
    StorageType NeedsUpdate : 1;
    StorageType Relocated : 1;
    StorageType InsideGeometry : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiDDGIProbeFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIProbeFlags);

/// Tool-facing state of one physical slot in the scrolling DDGI volume.
struct XII_GRAPHICSCORE_DLL xiiDDGIProbeState
{
  xiiInt32                       m_iCellX             = xiiMath::MaxValue<xiiInt32>();
  xiiInt32                       m_iCellY             = xiiMath::MaxValue<xiiInt32>();
  xiiInt32                       m_iCellZ             = xiiMath::MaxValue<xiiInt32>();
  xiiVec3                        m_vWorldPosition     = xiiVec3::MakeZero();
  xiiVec3                        m_vRelocationOffset  = xiiVec3::MakeZero();
  xiiUInt64                      m_uiLastUpdatedFrame = 0U;
  xiiBitflags<xiiDDGIProbeFlags> m_Flags              = xiiDDGIProbeFlags::NeedsUpdate;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIProbeState);

/// One probe selected for ray tracing and temporal integration this frame.
struct XII_GRAPHICSCORE_DLL xiiDDGIProbeUpdate
{
  xiiUInt32 m_uiPhysicalProbe = xiiInvalidIndex;
  xiiVec3   m_vWorldPosition  = xiiVec3::MakeZero();
  float     m_fHistoryWeight  = 0.0f;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIProbeUpdate);

struct XII_GRAPHICSCORE_DLL xiiDDGIFrameStats
{
  xiiUInt32 m_uiProbeCount           = 0U;
  xiiUInt32 m_uiInvalidProbeCount    = 0U;
  xiiUInt32 m_uiScheduledUpdateCount = 0U;
  xiiUInt32 m_uiScrolledProbeCount   = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiDDGIFrameStats);

struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuDDGIProbeState
{
  XII_DECLARE_POD_TYPE();

  xiiVec4    m_vPositionAndLastUpdate;
  xiiVec4I32 m_vCellAndFlags;
};

static_assert(sizeof(xiiGpuDDGIProbeState) == 32U);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuDDGIProbeState);

struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuDDGIProbeUpdate
{
  XII_DECLARE_POD_TYPE();

  xiiVec4    m_vPositionAndHistoryWeight;
  xiiVec4U32 m_vMetadata;
};

static_assert(sizeof(xiiGpuDDGIProbeUpdate) == 32U);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuDDGIProbeUpdate);

/// Process-wide DDGI clipmap scheduler.
///
/// The state is allocated during GraphicsCore subsystem startup, never from a
/// global constructor. Logical world cells map to stable toroidal physical
/// slots, preserving temporal history for the overlapping region when the
/// camera moves and invalidating only newly exposed probe slabs.
class XII_GRAPHICSCORE_DLL xiiDDGIManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDDGIManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, DDGIManager);

public:
  xiiDDGIManager() = delete;

  [[nodiscard]] static xiiResult Configure(const xiiDDGISettings& settings);
  [[nodiscard]] static bool      IsSubsystemInitialized();
  [[nodiscard]] static bool      IsInitialized();

  static void BeginFrame(const xiiVec3& vCameraPosition, xiiUInt64 uiFrameIndex);
  static void CommitProbeUpdate(xiiUInt32 uiPhysicalProbe, const xiiVec3& vRelocationOffset, bool bValid, bool bInsideGeometry = false);

  [[nodiscard]] static xiiArrayPtr<const xiiDDGIProbeState>  GetProbes();
  [[nodiscard]] static xiiArrayPtr<const xiiDDGIProbeUpdate> GetScheduledUpdates();
  [[nodiscard]] static xiiDDGIFrameStats                     GetFrameStats();
  [[nodiscard]] static const xiiDDGISettings&                GetConfiguration();

  struct UpdateHandles
  {
    xiiRenderGraphTextureHandle m_hIrradianceAtlas;
    xiiRenderGraphTextureHandle m_hDistanceAtlas;
    xiiRenderGraphBufferHandle  m_hProbeStates;
    xiiRenderGraphBufferHandle  m_hProbeConstants;
  };

  /// Adds the temporally accumulated probe update pass and publishes resources
  /// consumed by the per-pixel DDGI final gather.
  [[nodiscard]] static UpdateHandles AddUpdatePass(xiiRenderGraph& graph, const xiiLightingSystem* pLightingSystem);

private:
  static void                    Startup();
  static void                    EngineStartup();
  static void                    EngineShutdown();
  static void                    Shutdown();
  [[nodiscard]] static xiiResult CreateGpuResources();

  class State;
  static xiiUniquePtr<State> s_pState;
};
