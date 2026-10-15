/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Id.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Resources/BindlessResource.h>

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
  /// First meshlet in the source buffer that belongs to this LOD. This allows several LODs
  /// authored in one xiiMeshBufferResource to share their immutable vertex/remap buffers.
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
  xiiUInt32 m_uiVertexBufferIndex           = xiiInvalidIndex;
  xiiUInt32 m_uiIndexBufferIndex            = xiiInvalidIndex;
  xiiUInt32 m_uiMeshletBufferIndex          = xiiInvalidIndex;
  xiiUInt32 m_uiMeshletRemapBufferIndex     = xiiInvalidIndex;
  xiiUInt32 m_uiMeshletPrimitiveBufferIndex = xiiInvalidIndex;
  xiiUInt32 m_uiVertexCount                 = 0U;
  xiiUInt32 m_uiIndexCount                  = 0U;
  xiiUInt32 m_uiMeshletCount                = 0U;
  float     m_fMinimumScreenCoverage        = 0.0f;
  xiiUInt32 m_uiIndexType                   = 0U;
  xiiUInt32 m_uiMeshletMetadataOffset       = 0U;
  xiiUInt32 m_uiPadding                     = 0U;
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
  xiiUInt64 m_uiResidentBytes         = 0U;
  xiiUInt64 m_uiBudgetBytes           = 0U;
  xiiUInt64 m_uiUploadedBytes         = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryResidencyStats);

/// Startup configuration for the process-wide geometry residency service.
struct XII_GRAPHICSCORE_DLL xiiGeometryResidencyDescription
{
  xiiUInt32 m_uiMaxGeometries             = 65536U;
  xiiUInt32 m_uiFramesInFlight            = 3U;
  xiiUInt64 m_uiBudgetBytes               = 512ULL * 1024ULL * 1024ULL;
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
  /// Allocates shared device buffers on first consumption, on the render thread.
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

  /// Requests the finest required LOD. Concurrent view requests are merged toward the lowest
  /// LOD index and retain the strongest visibility priority until the missing range is resident.
  static void RequestResidency(xiiGeometryHandle handle, xiiUInt32 uiMinimumLod, xiiUInt64 uiFrameIndex, xiiUInt32 uiVisibilityPriority = 0U);
  static void Touch(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex);
  static void ProcessStreaming(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiUInt64 uiUploadBudgetBytes);

  [[nodiscard]] static bool                               IsValid(xiiGeometryHandle handle);
  [[nodiscard]] static xiiEnum<xiiGeometryResidencyState> GetState(xiiGeometryHandle handle);
  [[nodiscard]] static const xiiGpuGeometryRecord*        GetGpuRecord(xiiGeometryHandle handle);
  /// Counts meshlets overlapping a primitive range in each authored LOD, including unloaded LODs.
  /// A zero primitive count selects the entire LOD, matching GPU visibility. The output must have
  /// s_uiMaxLods entries and is cleared on failure. Reads source metadata in O(total meshlets);
  /// cache the result until the geometry handle or primitive range changes. Rejects reloaded
  /// source buffers belonging to a different revision. Call on the render thread.
  [[nodiscard]] static xiiResult                  GetPrimitiveRangeMeshletCounts(xiiGeometryHandle handle, xiiUInt32 uiFirstPrimitive, xiiUInt32 uiPrimitiveCount, xiiArrayPtr<xiiUInt32> out_counts);
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer> GetMetadataBuffer();
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

  struct Slot
  {
    static constexpr xiiUInt32 s_uiBindlessResourcesPerLod = 5U;

    xiiGeometryDescription             m_Description;
    xiiGpuGeometryRecord               m_GpuRecord;
    xiiEnum<xiiGeometryResidencyState> m_State                = xiiGeometryResidencyState::Unloaded;
    xiiUInt64                          m_uiLastUsedFrame      = 0U;
    xiiUInt64                          m_uiRetireFrame        = 0U;
    xiiUInt64                          m_uiResidentBytes      = 0U;
    xiiUInt32                          m_uiRequestedLod       = 0U;
    xiiUInt32                          m_uiVisibilityPriority = 0U;
    xiiUInt64                          m_uiFirstRequestFrame  = xiiMath::MaxValue<xiiUInt64>();
    bool                               m_bAllocated           = false;
    /// One bit per frame-in-flight metadata slice. A record change must reach every slice
    /// before it is considered clean; a single boolean would leave rotating slices stale.
    xiiUInt64 m_uiDirtyFrameMask                                        = 0U;
    xiiUInt32 m_uiMeshletArenaOffset[xiiGpuGeometryRecord::s_uiMaxLods] = {};
    xiiUInt32 m_uiMeshletArenaCount[xiiGpuGeometryRecord::s_uiMaxLods]  = {};
    /// Vertex, index, meshlet, remap and primitive SRVs owned by the residency service.
    /// Keeping the generation-bearing handles beside the LOD prevents callers from leaking
    /// descriptors or retiring them while an older frame still references the geometry record.
    xiiGALBindlessResourceHandle m_BindlessResources[xiiGpuGeometryRecord::s_uiMaxLods][s_uiBindlessResourcesPerLod] = {};
  };

  struct Upload
  {
    xiiUInt32            m_uiOffset    = 0U;
    xiiUInt32            m_uiSlotIndex = xiiInvalidIndex;
    xiiUInt64            m_uiFrameBit  = 0U;
    xiiGpuGeometryRecord m_Record;
  };

  struct UploadPassData
  {
    xiiRenderGraphBufferHandle                          m_hGeometryBuffer;
    xiiRenderGraphBufferHandle                          m_hMeshletBuffer;
    xiiDynamicArray<Upload, xiiAlignedAllocatorWrapper> m_Uploads;
    struct MeshletUpload
    {
      xiiUInt64                                               m_uiUploadId = 0U;
      xiiUInt32                                               m_uiOffset   = 0U;
      xiiDynamicArray<xiiMeshlet, xiiAlignedAllocatorWrapper> m_Meshlets;
    };
    xiiDynamicArray<MeshletUpload> m_MeshletUploads;
  };

  struct FreeRange
  {
    xiiUInt32 m_uiOffset = 0U;
    xiiUInt32 m_uiCount  = 0U;
  };

  static bool BuildResidentRecord(Slot& slot, xiiUInt64& inout_uiUploadBudget);
  static bool RegisterBindlessResources(Slot& slot, xiiGpuGeometryRecord& record, xiiUInt32 uiLod, xiiMeshBufferResource& mesh);
  static void ReleaseBindlessResources(Slot& slot, xiiUInt64 uiLastUseFrame);
  static void EnforceBudget(xiiUInt64 uiCompletedFrame);
  static bool AllocateMeshlets(xiiUInt32 uiCount, xiiUInt32& out_uiOffset);
  static void FreeMeshlets(xiiUInt32 uiOffset, xiiUInt32 uiCount);
  static void ReleaseMeshletAllocations(Slot& slot);

  class State;
  static xiiUniquePtr<State> s_pState;
};
