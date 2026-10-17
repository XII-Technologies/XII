/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

/// CPU-side residency configuration for the virtual shadow-map physical cache.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowMapSettings
{
  xiiUInt32 m_uiVirtualResolution     = 16384U;
  xiiUInt32 m_uiPageSize              = 128U;
  xiiUInt32 m_uiPhysicalPageCount     = 4096U;
  xiiUInt32 m_uiMaxFeedbackRequests   = 16384U;
  xiiUInt32 m_uiMaxPageAllocations    = 512U;
  xiiUInt32 m_uiMaxPageRasterizations = 128U;
  xiiUInt32 m_uiFramesInFlight        = 3U;
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

  [[nodiscard]] bool                          IsValid() const;
  [[nodiscard]] xiiUInt64                     GetPackedValue() const;
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

/// Tool-facing resident mapping and dirty state.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowPageMapping
{
  xiiVirtualShadowPageId m_Page;
  xiiUInt32              m_uiPhysicalPage  = xiiInvalidIndex;
  xiiUInt64              m_uiLastUsedFrame = 0U;
  xiiUInt32              m_uiPriority      = 0U;
  bool                   m_bPinned         = false;
  bool                   m_bNeedsRendering = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowPageMapping);

/// Frame-sliced physical-page metadata consumed by GPU shadow feedback and sampling.
struct XII_GRAPHICSCORE_DLL xiiGpuVirtualShadowPage
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiVirtualKeyLow  = 0U;
  xiiUInt32 m_uiVirtualKeyHigh = 0U;
  xiiUInt32 m_uiPhysicalPage   = xiiInvalidIndex;
  xiiUInt32 m_uiFlags          = 0U; ///< bit 0 resident, bit 1 needs rendering, bit 2 pinned.
};

static_assert(sizeof(xiiGpuVirtualShadowPage) == 16U, "Virtual shadow page records must match the GPU structured-buffer layout.");
static_assert(alignof(xiiGpuVirtualShadowPage) <= 8U, "Virtual shadow page records must remain compatible with the default engine allocator.");

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuVirtualShadowPage);

struct XII_GRAPHICSCORE_DLL xiiVirtualShadowMapStats
{
  XII_DECLARE_POD_TYPE();

  xiiUInt32 m_uiResidentPageCount        = 0U;
  xiiUInt32 m_uiFreePageCount            = 0U;
  xiiUInt32 m_uiRetiredPageCount         = 0U;
  xiiUInt32 m_uiFeedbackRequestCount     = 0U;
  xiiUInt32 m_uiUniqueRequestCount       = 0U;
  xiiUInt32 m_uiAllocationCount          = 0U;
  xiiUInt32 m_uiEvictionCount            = 0U;
  xiiUInt32 m_uiDroppedRequestCount      = 0U;
  xiiUInt32 m_uiInvalidationCount        = 0U;
  xiiUInt32 m_uiRasterizedPageCount      = 0U;
  xiiUInt32 m_uiDirtyPageCount           = 0U;
  xiiUInt32 m_uiPhysicalAtlasWidth       = 0U;
  xiiUInt32 m_uiPhysicalAtlasHeight      = 0U;
  xiiUInt32 m_uiVirtualPageTableCapacity = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiVirtualShadowMapStats);

/// Immutable frame-local raster request for one physical residency page. The revision
/// validates its contents across invalidation/reuse; it is never a serialized identity.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowRasterPage
{
  xiiVirtualShadowPageId m_Page;
  xiiUInt32              m_uiPhysicalPage    = xiiInvalidIndex;
  xiiUInt64              m_uiContentRevision = 0;
  xiiUInt64              m_uiFrame           = 0;
  xiiMat4                m_ViewProjection    = xiiMat4::MakeIdentity();
  xiiRectU32             m_Viewport;
};

/// Device table addressing for one retired-frame-safe slice. Derived residency
/// metadata only; these indices are never public scene identities or serialized handles.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowGpuLayout
{
  xiiUInt32 m_uiFrameBaseIndex = 0, m_uiPhysicalPageCount = 0;
  xiiUInt32 m_uiVirtualTableBaseIndex = 0, m_uiVirtualTableCapacity = 0;
};

/// Immutable upload capture. Owns the referenced GPU allocations across reconfiguration.
/// Table transfers are recorded by the consuming pipeline operation.
struct XII_GRAPHICSCORE_DLL xiiVirtualShadowTableUpload : xiiRefCounted
{
  struct Range
  {
    xiiUInt32 m_uiFirstPhysicalPage = 0, m_uiFirstRecord = 0, m_uiRecordCount = 0;
  };
  xiiSharedPtr<xiiGALBuffer>               m_PhysicalBuffer, m_VirtualBuffer;
  xiiSharedPtr<xiiGALTexture>              m_Atlas;
  xiiVirtualShadowGpuLayout                m_Layout;
  xiiDynamicArray<Range>                   m_PhysicalRanges;
  xiiDynamicArray<xiiGpuVirtualShadowPage> m_PhysicalRecords, m_VirtualRecords;
  xiiDynamicArray<xiiUInt64>               m_PhysicalRevisions;
  xiiUInt64                                m_uiFrame = 0, m_uiConfigurationRevision = 0, m_uiResidencyRevision = 0;
};

class xiiVirtualShadowMapManagerState;

/// Process-wide virtual-shadow-map page residency service.
///
/// State is allocated by the GraphicsCore subsystem after Foundation startup,
/// avoiding allocator use from global constructors. Owns device page allocations,
/// CPU demand and residency metadata. Feedback and raster algorithms belong to selected pipeline
/// resources. Access is serialized by the existing GPU frame owner.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowMapManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiVirtualShadowMapManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, VirtualShadowMapManager);

public:
  xiiVirtualShadowMapManager() = delete;

  [[nodiscard]] static xiiResult Configure(const xiiVirtualShadowMapSettings& settings);
  [[nodiscard]] static bool      IsSubsystemInitialized();
  [[nodiscard]] static bool      IsInitialized();

  /// Starts a residency frame. completedFrame is the newest frame whose GPU
  /// work has retired; only pages last used by that frame may be evicted.
  static void BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame);

  /// Deduplicates, prioritizes, touches, allocates, and safely evicts pages.
  static void SubmitFeedback(xiiArrayPtr<const xiiVirtualShadowPageRequest> requests);

  [[nodiscard]] static bool                                           TryGetMapping(const xiiVirtualShadowPageId& page, xiiVirtualShadowPageMapping& out_mapping);
  [[nodiscard]] static xiiArrayPtr<const xiiVirtualShadowPageMapping> GetDirtyPages();
  /// Marks one resident page dirty after an occluder, light, or projection change.
  /// Call before graph setup; the page remains sampleable in the same frame once
  /// the selected raster graph and page completion have written the atlas/table.
  [[nodiscard]] static bool InvalidatePage(const xiiVirtualShadowPageId& page);
  /// Invalidates resident pages inside a virtual page-space rectangle.
  [[nodiscard]] static xiiUInt32 InvalidateRegion(xiiUInt32 uiLightId, xiiUInt32 uiMipLevel, const xiiRectU32& pageRegion);
  /// Invalidates every resident page owned by one stable light identifier.
  [[nodiscard]] static xiiUInt32 InvalidateLight(xiiUInt32 uiLightId);
  /// Invalidates all resident pages, for example after a scene-wide origin shift.
  [[nodiscard]] static xiiUInt32 InvalidateAll();
  /// Reports proven GPU completion, rejecting invalidated/recycled content revisions.
  [[nodiscard]] static bool MarkPageRendered(const xiiVirtualShadowRasterPage& page);

  [[nodiscard]] static xiiVirtualShadowMapStats           GetStats();
  [[nodiscard]] static const xiiVirtualShadowMapSettings& GetConfiguration();
  /// Transient device residency epoch. Rejects feedback captured before reconfiguration.
  [[nodiscard]] static xiiUInt64 GetConfigurationRevision();

  /// Persistent depth atlas owned by the subsystem. Consumers import this
  /// resource into their render graph but never own or destroy it.
  [[nodiscard]] static xiiSharedPtr<xiiGALTexture> GetPhysicalAtlas();

  /// Returns the texel viewport assigned to a physical page.
  [[nodiscard]] static bool GetPhysicalPageViewport(xiiUInt32 uiPhysicalPage, xiiRectU32& out_viewport);

  /// Crops one cascade view-projection matrix to the requested virtual page.
  /// The resulting clip space fills a physical page viewport without changing
  /// reversed-Z depth. Exposed for tooling and deterministic validation.
  [[nodiscard]] static bool BuildPageViewProjection(const xiiMat4& cascadeViewProjection, const xiiVirtualShadowPageId& page, xiiUInt32 uiVirtualResolution, xiiUInt32 uiPageSize, xiiMat4& out_pageViewProjection);

  /// Captures coalesced physical edits and a changed virtual table for the current
  /// frame slice. The first capture freezes this device frame; later residency changes are
  /// queued for the next frame rather than rewriting tables under active views.
  [[nodiscard]] static xiiSharedPtr<const xiiVirtualShadowTableUpload> CaptureTableUpload(xiiUInt64 uiFrameIndex);
  /// Register a recorded transfer. Dirty edits retire only after its fence signals;
  /// abandoned/failed recordings remain retryable after shared frame retirement.
  static void               RecordTableUpload(xiiSharedPtr<const xiiVirtualShadowTableUpload> upload, xiiSharedPtr<xiiGALFence> completion);
  [[nodiscard]] static bool IsTableUploadRecorded(const xiiVirtualShadowTableUpload& upload);

  /// Select a deterministic bounded batch without constructing any raster operations.
  /// Selected pages are touched for GPU retirement and claimed once per device frame.
  static void               GatherRasterPages(xiiArrayPtr<const xiiMat4> cascadeViewProjections, xiiUInt32 uiLightId,
                                              xiiDynamicArray<xiiVirtualShadowRasterPage>& out_pages);
  [[nodiscard]] static bool IsRasterWorkCurrent(const xiiVirtualShadowRasterPage& page);
  /// Publish clean GPU table records after the supplied final atlas version. CPU dirty
  /// state is retired only after the completion fence signals, and abandoned work retries.
  [[nodiscard]] static xiiResult AddPageCompletionPass(xiiRenderGraph& graph, const xiiVirtualShadowGpuLayout& layout,
                                                       xiiRenderGraphTextureHandle& atlas, xiiRenderGraphBufferHandle& table,
                                                       xiiArrayPtr<const xiiVirtualShadowRasterPage> pages);


private:
  static void                                          AcknowledgeTableUpload(const xiiVirtualShadowTableUpload& upload);
  static void                                          Startup();
  static void                                          EngineStartup();
  static void                                          EngineShutdown();
  static void                                          Shutdown();
  [[nodiscard]] static xiiResult                       CreateGpuResources();
  static xiiUniquePtr<xiiVirtualShadowMapManagerState> s_pState;
};
