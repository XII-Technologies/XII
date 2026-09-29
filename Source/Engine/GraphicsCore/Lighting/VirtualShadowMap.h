/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// CPU-side residency configuration for the virtual shadow-map physical cache.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowMapSettings
{
  xiiUInt32 m_uiVirtualResolution      = 16384U;
  xiiUInt32 m_uiPageSize               = 128U;
  xiiUInt32 m_uiPhysicalPageCount      = 4096U;
  xiiUInt32 m_uiMaxFeedbackRequests    = 16384U;
  xiiUInt32 m_uiMaxPageAllocations     = 512U;
  xiiUInt32 m_uiFramesInFlight         = 3U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowMapSettings);

/// Stable virtual address of one shadow-map page.
///
/// The packed representation reserves 24 bits for the light, 6 for the mip,
/// and 17 bits for each page coordinate. It is deterministic across frames and
/// can be written directly to GPU feedback and page-table update buffers.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowPageId
{
  XII_DECLARE_POD_TYPE();

  [[nodiscard]] bool      IsValid() const;
  [[nodiscard]] xiiUInt64 GetPackedValue() const;
  [[nodiscard]] static xiiVirtualShadowPageId FromPackedValue(xiiUInt64 uiPackedValue);

  xiiUInt32 m_uiLightId  = xiiInvalidIndex;
  xiiUInt32 m_uiMipLevel = 0U;
  xiiUInt32 m_uiPageX    = 0U;
  xiiUInt32 m_uiPageY    = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowPageId);

/// One unique page request produced by compacted GPU shadow feedback.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowPageRequest
{
  xiiVirtualShadowPageId m_Page;
  xiiUInt32              m_uiPriority = 0U;
  bool                   m_bPinned    = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowPageRequest);

struct XII_GRAPHICSCORE_DLL xiiVirtualShadowPageUpdateType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Map,
    Unmap,
    Default = Map
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowPageUpdateType);

/// Incremental page-table edit consumed by the GPU page-table update pass.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowPageUpdate
{
  xiiVirtualShadowPageId                   m_Page;
  xiiUInt32                               m_uiPhysicalPage = xiiInvalidIndex;
  xiiEnum<xiiVirtualShadowPageUpdateType> m_Type;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowPageUpdate);

/// Tool-facing resident mapping and dirty state.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowPageMapping
{
  xiiVirtualShadowPageId m_Page;
  xiiUInt32              m_uiPhysicalPage  = xiiInvalidIndex;
  xiiUInt64              m_uiLastUsedFrame = 0U;
  xiiUInt32              m_uiPriority = 0U;
  bool                   m_bPinned = false;
  bool                   m_bNeedsRendering = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowPageMapping);

/// Frame-sliced physical-page metadata consumed by GPU shadow feedback and sampling.
struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuVirtualShadowPage
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiVirtualKeyLow  = 0U;
  xiiUInt32 m_uiVirtualKeyHigh = 0U;
  xiiUInt32 m_uiPhysicalPage   = xiiInvalidIndex;
  xiiUInt32 m_uiFlags          = 0U; ///< bit 0 resident, bit 1 needs rendering, bit 2 pinned.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVirtualShadowPage);

/// GPU feedback record. Element zero uses VirtualKeyLow as the append counter.
struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuVirtualShadowFeedback
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiVirtualKeyLow  = 0U;
  xiiUInt32 m_uiVirtualKeyHigh = 0U;
  xiiUInt32 m_uiPriority       = 0U;
  xiiUInt32 m_uiFlags          = 0U; ///< bit 0 requests pinned residency.
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVirtualShadowFeedback);

struct XII_GRAPHICSCORE_DLL xiiVirtualShadowMapStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiResidentPageCount    = 0U;
  xiiUInt32 m_uiFreePageCount        = 0U;
  xiiUInt32 m_uiFeedbackRequestCount = 0U;
  xiiUInt32 m_uiUniqueRequestCount   = 0U;
  xiiUInt32 m_uiAllocationCount      = 0U;
  xiiUInt32 m_uiEvictionCount        = 0U;
  xiiUInt32 m_uiDroppedRequestCount  = 0U;
  xiiUInt32 m_uiDirtyPageCount       = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowMapStats);

class xiiVirtualShadowMapManagerState;

/// Process-wide virtual-shadow-map page residency service.
///
/// State is allocated by the GraphicsCore subsystem after Foundation startup,
/// avoiding allocator use from global constructors. The manager is CPU-only;
/// render-graph passes upload GetPageTableUpdates() and render GetDirtyPages().
class XII_GRAPHICSCORE_DLL xiiVirtualShadowMapManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiVirtualShadowMapManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, VirtualShadowMapManager);

public:
  xiiVirtualShadowMapManager() = delete;

  [[nodiscard]] static xiiResult Configure(const xiiVirtualShadowMapSettings& settings);
  [[nodiscard]] static bool      IsInitialized();

  /// Starts a residency frame. completedFrame is the newest frame whose GPU
  /// work has retired; only pages last used by that frame may be evicted.
  static void BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame);

  /// Deduplicates, prioritizes, touches, allocates, and safely evicts pages.
  static void SubmitFeedback(xiiArrayPtr<const xiiVirtualShadowPageRequest> requests);

  [[nodiscard]] static bool TryGetMapping(const xiiVirtualShadowPageId& page, xiiVirtualShadowPageMapping& out_mapping);
  [[nodiscard]] static xiiArrayPtr<const xiiVirtualShadowPageUpdate> GetPageTableUpdates();
  [[nodiscard]] static xiiArrayPtr<const xiiVirtualShadowPageMapping> GetDirtyPages();
  static void                      MarkPageRendered(xiiUInt32 uiPhysicalPage);

  [[nodiscard]] static xiiVirtualShadowMapStats GetStats();
  [[nodiscard]] static const xiiVirtualShadowMapSettings& GetConfiguration();

  struct UploadHandles
  {
    xiiRenderGraphBufferHandle m_hPhysicalPageTable;
    xiiUInt32                  m_uiFrameBaseIndex = 0U;
    xiiUInt32                  m_uiPhysicalPageCount = 0U;
  };

  /// Adds a transfer pass that publishes changed residency records to the
  /// current frame slice. Updates remain retryable if graph execution fails.
  [[nodiscard]] static UploadHandles AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex);

  /// Appends screen-derived directional shadow requests and copies the cumulative
  /// feedback stream into the completed-frame readback ring.
  static void AddFeedbackPasses(xiiRenderGraph& graph, xiiRenderGraphTextureHandle hSceneDepth,
    xiiRenderGraphBufferHandle hCascadeConstants, xiiUInt32 uiWidth, xiiUInt32 uiHeight,
    xiiUInt32 uiDirectionalLightId, xiiUInt64 uiFrameIndex);

private:
  static void Startup();
  static void EngineStartup();
  static void EngineShutdown();
  static void Shutdown();
  [[nodiscard]] static xiiResult CreateGpuResources();
  static xiiUniquePtr<xiiVirtualShadowMapManagerState> s_pState;
};
