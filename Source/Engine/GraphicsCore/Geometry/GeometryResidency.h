/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Resources/BindlessResource.h>

class xiiVirtualTextureData;

struct XII_GRAPHICSCORE_DLL xiiGeometryResidencyState
{
  using StorageType = xiiUInt8;
  enum Enum : StorageType
  {
    Unloaded,
    Requested,
    Loading,
    Resident,
    EvictPending,
    Failed,
    Default = Unloaded
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryResidencyState);

/// Generation-checked reference to one subsystem-owned geometry record.
using xiiGeometryId = xiiGenericId<24, 8>;

class XII_GRAPHICSCORE_DLL xiiGeometryHandle
{
  XII_DECLARE_HANDLE_TYPE(xiiGeometryHandle, xiiGeometryId);
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryHandle);

struct XII_GRAPHICSCORE_DLL xiiGeometryLodSource
{
  [[nodiscard]] xiiString GetMeshBufferResourceId() const;
  void                    SetMeshBufferResourceId(xiiString sResourceId);

  xiiMeshBufferResourceHandle m_hMeshBuffer;
  float                       m_fMinimumScreenCoverage = 0.0f;
  /// First source meshlet in this LOD. LODs share canonical vertex and index buffers.
  /// Residency uploads and binds topology for this meshlet range.
  xiiUInt32 m_uiFirstMeshlet = 0U;
  /// Number of source meshlets in this LOD. Zero consumes every meshlet after m_uiFirstMeshlet.
  xiiUInt32 m_uiMeshletCount = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryLodSource);

struct XII_GRAPHICSCORE_DLL xiiGeometryDescription
{
  xiiHybridArray<xiiGeometryLodSource, 8U> m_Lods;
  xiiUInt32                                m_uiStreamingPriority = 0U;
  bool                                     m_bPinned             = false;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryDescription);

/// One LOD consumed by GPU LOD selection and command generation.
struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuGeometryLod
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiVertexBufferIndex          = xiiInvalidIndex;
  xiiUInt32 m_uiIndexBufferIndex           = xiiInvalidIndex;
  xiiUInt32 m_uiPadding                    = 0U;
  xiiUInt32 m_uiMeshletTopologyBufferIndex = xiiInvalidIndex;
  xiiUInt32 m_uiMeshletPrimitiveByteOffset = 0U;
  xiiUInt32 m_uiVertexCount                = 0U;
  xiiUInt32 m_uiIndexCount                 = 0U;
  xiiUInt32 m_uiMeshletCount               = 0U;
  float     m_fMinimumScreenCoverage       = 0.0f;
  xiiUInt32 m_uiIndexType                  = 0U;
  xiiUInt32 m_uiMeshletMetadataOffset      = 0U;
  xiiUInt32 m_uiMeshletRangesOrdered       = 0U; ///< Primitive-sorted arena metadata with UInt32 prefix ends in GPU padding[0].
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuGeometryLod);

struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuGeometryRecord
{
  XII_DECLARE_POD_TYPE();
  static constexpr xiiUInt32 s_uiMaxLods = 8U;

  xiiVec4           m_BoundsCenterRadius = xiiVec4::MakeZero();
  xiiVec4           m_BoundsExtents      = xiiVec4::MakeZero();
  xiiGpuGeometryLod m_Lods[s_uiMaxLods];
  xiiUInt32         m_uiLodCount        = 0U;
  xiiUInt32         m_uiGeneration      = 0U;
  xiiUInt32         m_uiResidentLodMask = 0U;
  xiiUInt32         m_uiFlags           = 0U;

  [[nodiscard]] XII_ALWAYS_INLINE xiiUInt32         GetReflectedLodCount() const { return m_uiLodCount; }
  [[nodiscard]] XII_ALWAYS_INLINE xiiGpuGeometryLod GetReflectedLod(xiiUInt32 uiIndex) const { return m_Lods[uiIndex]; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGpuGeometryRecord);

struct XII_GRAPHICSCORE_DLL xiiGeometryResidencyStats
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiGeometryCount         = 0U;
  xiiUInt32 m_uiResidentGeometryCount = 0U;
  xiiUInt32 m_uiPendingGeometryCount  = 0U;
  xiiUInt64 m_uiGeometryPageBytes = 0U, m_uiTexturePageBytes = 0U;
  xiiUInt32 m_uiGeometryPages = 0U, m_uiTexturePages = 0U;
  xiiUInt64 m_uiPageRequests = 0U, m_uiPageMisses = 0U;
  xiiUInt64 m_uiGeometryPageRequests = 0U, m_uiGeometryPageUploads = 0U;
  xiiUInt32 m_uiPendingPageRequests = 0U, m_uiPendingPageIO = 0U;
  xiiUInt64 m_uiResidentBytes         = 0U; ///< Occupied meshlet metadata arena bytes.
  xiiUInt64 m_uiBudgetBytes           = 0U; ///< Soft meshlet metadata occupancy limit.
  xiiUInt64 m_uiUploadedBytes         = 0U;
  xiiUInt64 m_uiAdmittedBytes         = 0U; ///< Metadata and topology transfer bytes charged this frame.
  xiiUInt64 m_uiResidentTopologyBytes = 0U; ///< Unique live LOD topology payload bytes, including word padding.
  xiiUInt64 m_uiTopologyUploadBytes   = 0U; ///< Topology bytes submitted by admissions this frame.
  xiiUInt64 m_uiRetiredTopologyBytes  = 0U; ///< Payload bytes passed to deferred retirement this frame.
  xiiUInt64 m_uiReclaimedArenaBytes   = 0U; ///< Arena bytes made reusable by evictions this frame.
  xiiUInt32 m_uiEvictions             = 0U;
  xiiUInt32 m_uiPressureEvictions     = 0U;
  xiiUInt64 m_uiRemainingUploadBytes  = 0U;
  xiiUInt64 m_uiPreparedMeshlets      = 0U;
  xiiUInt32 m_uiExamined = 0U, m_uiAdmitted = 0U, m_uiBudgetDeferred = 0U;
  xiiUInt32 m_uiNotReady = 0U, m_uiArenaDeferred = 0U, m_uiMemoryDeferred = 0U, m_uiFailed = 0U;
  xiiUInt32 m_uiArenaMeshlets        = 0U;
  double    m_fStreamingMilliseconds = 0.0;
  xiiUInt64 m_uiCoverageQueries      = 0U;
  xiiUInt64 m_uiCoverageCacheHits    = 0U;
  xiiUInt64 m_uiCoverageIndexBuilds  = 0U;
  xiiUInt64 m_uiCoverageMeshlets     = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryResidencyStats);

/// Startup configuration for the process-wide geometry residency service.
struct XII_GRAPHICSCORE_DLL xiiGeometryResidencyDescription
{
  xiiUInt32 m_uiMaxGeometries             = 65536U;
  xiiUInt32 m_uiFramesInFlight            = 3U;
  xiiUInt64 m_uiBudgetBytes               = 512ULL * 1024ULL * 1024ULL; ///< Soft limit for occupied meshlet metadata.
  xiiUInt64 m_uiUploadBudgetPerFrameBytes = 32ULL * 1024ULL * 1024ULL;
  xiiUInt32 m_uiMaxMeshlets               = 1024U * 1024U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryResidencyDescription);

/// Static subsystem facade for the stable GPU geometry table and mesh LOD residency service.
///
/// Allocator-backed CPU state is created during core startup and GPU resources are created during
/// high-level startup. Callers never construct, own, cache, or destroy a manager object.
class XII_GRAPHICSCORE_DLL xiiGeometryResidencyManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGeometryResidencyManager);
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(GraphicsCore, GeometryResidencyManager);

public:
  xiiGeometryResidencyManager() = delete;

  /// Stores startup configuration or reapplies it while no geometry handles are active.
  [[nodiscard]] static xiiResult                              Configure(const xiiGeometryResidencyDescription& description);
  [[nodiscard]] static const xiiGeometryResidencyDescription& GetConfiguration();
  [[nodiscard]] static bool                                   IsSubsystemInitialized();
  [[nodiscard]] static bool                                   IsInitialized();
  /// Allocates shared device buffers on first consumption. Concurrent initialization requests
  /// share one completed allocation; subsequent residency mutations use the render-frame scope.
  [[nodiscard]] static xiiResult EnsureInitialized();

  [[nodiscard]] static xiiGeometryHandle RegisterGeometry(const xiiGeometryDescription& description);
  static void                            UnregisterGeometry(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex);

  /// Acquires a shared residency record for an engine mesh resource. Multiple scene objects using
  /// the same content revision receive the same generation-checked geometry handle. Reloading
  /// publishes a new record; outstanding references retain the previous revision. Mesh LODs packed
  /// into one buffer are translated into reflected meshlet ranges by the subsystem. Call on the
  /// render thread, as with other residency mutations. Returns an invalid handle for unloaded data.
  [[nodiscard]] static xiiGeometryHandle AcquireMeshGeometry(const xiiMeshResourceHandle& hMesh);
  /// Releases the exact revision acquired through AcquireMeshGeometry(). After its last reference,
  /// the record and its GPU allocations remain valid until its last-use frame has completed.
  static void ReleaseMeshGeometry(xiiGeometryHandle hGeometry, xiiUInt64 uiFrameIndex);

  /// Admits immutable selection metadata and guaranteed root pages for all authored LODs.
  static void                    RequestGeometry(xiiGeometryHandle handle, xiiUInt64 uiFrame, xiiUInt32 uiPriority = 0U);
  [[nodiscard]] static xiiUInt32 ResolveVirtualTexture(xiiSharedPtr<xiiVirtualTextureData> data, xiiUInt64 uiFrame);
  static void                    EndStreamingFrame(xiiUInt64 uiFrame);
  static void                    Touch(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex);
  /// The first invocation in a device frame establishes its shared allowance. Further calls
  /// consume the remainder, including newly discovered geometry. One indivisible record may
  /// exceed an unused positive allowance. Zero pauses admission for the entire frame.
  static void ProcessStreaming(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiUInt64 uiUploadBudgetBytes);

  [[nodiscard]] static bool                               IsValid(xiiGeometryHandle handle);
  [[nodiscard]] static xiiEnum<xiiGeometryResidencyState> GetState(xiiGeometryHandle handle);
  [[nodiscard]] static const xiiGpuGeometryRecord*        GetGpuRecord(xiiGeometryHandle handle);
  /// Counts meshlets overlapping a primitive range in each authored LOD, including unloaded LODs.
  /// A zero primitive count selects the entire LOD, matching GPU visibility. The output must have
  /// s_uiMaxLods entries and is cleared on failure. Builds a revision-owned interval index once,
  /// then counts exact overlaps in O(LODs * log(meshlets)). Rejects reloaded
  /// source buffers belonging to a different revision. Call on the render thread.
  [[nodiscard]] static xiiResult                  GetPrimitiveRangeMeshletCounts(xiiGeometryHandle handle, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiPrimitiveCount, xiiArrayPtr<xiiUInt32> out_counts);
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer> GetMetadataBuffer();
  /// Shader-visible first element in the shared metadata frame slice.
  [[nodiscard]] static xiiUInt32                  GetMetadataBaseIndex(xiiUInt64 frameIndex);
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer> GetMeshletMetadataBuffer();
  [[nodiscard]] static xiiGeometryResidencyStats  GetStats();

  struct UploadHandles
  {
    xiiRenderGraphBufferHandle m_hGeometryMetadata;
    xiiRenderGraphBufferHandle m_hMeshletMetadata;
    /// First geometry record in the frame slice uploaded by AddUploadPass().
    xiiUInt32 m_uiGeometryBaseIndex = 0U;
    /// Largest meshlet count in any currently resident LOD. Visibility uses this to
    /// derive a complete indirect dispatch instead of trusting an authored estimate.
    xiiUInt32 m_uiMaximumResidentMeshletCount = 0U;
  };

  [[nodiscard]] static UploadHandles AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex);

private:
  static void      Startup();
  static xiiResult Initialize(xiiGALDevice* pDevice, const xiiGeometryResidencyDescription& description);
  static void      Shutdown();
  static void      EngineStartup();
  static void      EngineShutdown();

  struct CoverageIndex
  {
    struct Lod
    {
      xiiDynamicArray<xiiUInt32> m_Starts;
      xiiDynamicArray<xiiUInt64> m_Ends;
      xiiUInt32                  m_uiRevision = 0U;
      bool                       m_bValid     = false;
    };
    Lod m_Lods[xiiGpuGeometryRecord::s_uiMaxLods];
  };

  struct MeshletUpload : xiiRefCounted
  {
    xiiUInt32                                               m_uiSlotIndex = 0U, m_uiLod = 0U;
    xiiUInt32                                               m_uiOffset = 0U;
    xiiDynamicArray<xiiMeshlet, xiiAlignedAllocatorWrapper> m_Meshlets;
    bool                                                    m_bRecorded = false;
    bool                                                    m_bCanceled = false;
  };

  struct LodBinding
  {
    const void* m_pSource = nullptr;
  };

  struct Slot
  {
    xiiGeometryDescription             m_Description;
    xiiGpuGeometryRecord               m_GpuRecord;
    xiiEnum<xiiGeometryResidencyState> m_State                = xiiGeometryResidencyState::Unloaded;
    xiiUInt64                          m_uiLastUsedFrame      = 0U;
    xiiUInt64                          m_uiRetireFrame        = 0U;
    xiiUInt64                          m_uiResidentBytes      = 0U;
    xiiUInt32                          m_uiVisibilityPriority = 0U;
    xiiUInt64                          m_uiFirstRequestFrame  = xiiMath::MaxValue<xiiUInt64>();
    xiiUInt64                          m_uiAdmissionFrame     = xiiMath::MaxValue<xiiUInt64>();
    bool                               m_bAllocated           = false;
    bool                               m_bQueued              = false;
    xiiUInt32                          m_uiActiveIndex        = 0U;
    xiiSharedPtr<MeshletUpload>        m_PendingUploads[xiiGpuGeometryRecord::s_uiMaxLods];
    xiiUniquePtr<CoverageIndex>        m_pCoverage;
    /// One bit per frame-in-flight metadata slice. A record change must reach every slice
    /// before it is considered clean; a single boolean would leave rotating slices stale.
    xiiUInt64 m_uiDirtyFrameMask                                        = 0U;
    xiiUInt32 m_uiMeshletArenaOffset[xiiGpuGeometryRecord::s_uiMaxLods] = {};
    xiiUInt32 m_uiMeshletArenaCount[xiiGpuGeometryRecord::s_uiMaxLods]  = {};
    /// Authored selection metadata leases one revision-owned source representation.
    LodBinding m_BoundSources[xiiGpuGeometryRecord::s_uiMaxLods];
  };

  struct Upload
  {
    xiiUInt32 m_uiOffset    = 0U;
    xiiUInt32 m_uiSlotIndex = xiiInvalidIndex;
    xiiUInt64 m_uiFrameBit  = 0U;
  };

  struct UploadPassData
  {
    xiiRenderGraphBufferHandle                                        m_hGeometryBuffer;
    xiiRenderGraphBufferHandle                                        m_hMeshletBuffer;
    xiiDynamicArray<Upload, xiiAlignedAllocatorWrapper>               m_Uploads;
    xiiDynamicArray<xiiGpuGeometryRecord, xiiAlignedAllocatorWrapper> m_Records;
    xiiDynamicArray<xiiSharedPtr<MeshletUpload>>                      m_MeshletUploads;
  };

  struct FreeRange
  {
    xiiUInt32 m_uiOffset = 0U;
    xiiUInt32 m_uiCount  = 0U;
  };

  enum class AdmissionResult
  {
    Admitted,
    BudgetDeferred,
    NotReady,
    ArenaDeferred,
    MemoryDeferred,
    Failed
  };
  static AdmissionResult BuildResidentRecord(Slot& slot, xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame);
  static AdmissionResult RegisterBindlessResources(Slot& slot, xiiGpuGeometryRecord& record, xiiUInt32 uiLod, const xiiMeshBufferResource& mesh, const xiiSharedPtr<xiiGALBuffer>& pVertexBuffer);
  static void            ReleaseSourceBinding(LodBinding binding, xiiUInt64 uiLastUseFrame);
  static void            ReleaseBindlessResources(Slot& slot, xiiUInt64 uiLastUseFrame);
  static void            EnforceBudget(xiiUInt64 uiCompletedFrame, xiiUInt64 uiRequiredBytes = 0U);
  static bool            EvictOne(xiiUInt64 uiCompletedFrame, xiiUInt64 uiMinimumIdleFrames = 1U);
  static bool            AllocateMeshlets(xiiUInt32 uiCount, xiiUInt32& out_uiOffset);
  static void            FreeMeshlets(xiiUInt32 uiOffset, xiiUInt32 uiCount);
  static void            ReleaseMeshletAllocations(Slot& slot);

  class State;
  static xiiSharedPtr<State> s_pState;
};
