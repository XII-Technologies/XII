/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

class xiiGALDevice;

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

/// Camera-centred GI algorithm state for one consumer. CPU scheduling can be used without a device.
/// GPU storage is private to the context; settings and algorithm state are never process global.
/// This object is not thread-safe. It must outlive graphs capturing it. Before reinitialization,
/// shutdown or destruction, discard those graphs and wait for their submitted GPU work to finish.
class XII_GRAPHICSCORE_DLL xiiDDGIContext
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiDDGIContext);

public:
  xiiDDGIContext();
  ~xiiDDGIContext();

  /// Initialize CPU scheduling and, when a device is supplied, persistent GPU storage.
  /// Failure preserves an existing configuration. A null device selects CPU-only scheduling.
  [[nodiscard]] xiiResult Initialize(const xiiDDGISettings& settings = {}, xiiSharedPtr<xiiGALDevice> pDevice = nullptr);
  void                    Shutdown();
  [[nodiscard]] bool      IsInitialized() const;
  /// Discard accumulated lighting without reallocating storage or disturbing another view.
  void InvalidateHistory();

  /// Frames must be nonzero and strictly increasing; the entire clipmap must fit its cell coordinates.
  /// Non-finite or out-of-range positions and invalid frames fail without changing state.
  [[nodiscard]] xiiResult BeginFrame(const xiiVec3& vCameraPosition, xiiUInt64 uiFrameIndex);
  void                    CommitProbeUpdate(xiiUInt32 uiPhysicalProbe, const xiiVec3& vRelocationOffset, bool bValid, bool bInsideGeometry = false);

  [[nodiscard]] xiiArrayPtr<const xiiDDGIProbeState>  GetProbes() const;
  [[nodiscard]] xiiArrayPtr<const xiiDDGIProbeUpdate> GetScheduledUpdates() const;
  [[nodiscard]] xiiDDGIFrameStats                     GetFrameStats() const;
  [[nodiscard]] const xiiDDGISettings&                GetConfiguration() const;

  struct UpdateHandles
  {
    xiiRenderGraphTextureHandle m_hIrradianceAtlas;
    xiiRenderGraphTextureHandle m_hDistanceAtlas;
    xiiRenderGraphBufferHandle  m_hProbeStates;
    xiiRenderGraphBufferHandle  m_hProbeConstants;
  };

  /// Adds the temporally accumulated probe update pass and publishes resources
  /// consumed by the per-pixel DDGI final gather.
  [[nodiscard]] UpdateHandles AddUpdatePass(xiiRenderGraph& graph, xiiRenderGraphBufferHandle environmentLight);

private:
  [[nodiscard]] xiiResult CreateGpuResources();
  class State;
  xiiUniquePtr<State> m_pState;
};
