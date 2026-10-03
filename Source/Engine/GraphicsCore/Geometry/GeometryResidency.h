/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Containers/HybridArray.h>
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

struct XII_GRAPHICSCORE_DLL xiiGeometryHandle
{
  XII_DECLARE_POD_TYPE();
  [[nodiscard]] XII_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != xiiInvalidIndex && m_uiGeneration != 0U; }
  xiiUInt32                            m_uiIndex      = xiiInvalidIndex;
  xiiUInt32                            m_uiGeneration = 0U;
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

  [[nodiscard]] static xiiGeometryHandle RegisterGeometry(const xiiGeometryDescription& description);
  static void                            UnregisterGeometry(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex);

  /// Acquires a shared residency record for an engine mesh resource. Multiple scene objects using
  /// the same resource receive the same generation-checked geometry handle. Mesh LODs packed into
  /// one buffer are translated into reflected meshlet ranges by the subsystem.
  [[nodiscard]] static xiiGeometryHandle AcquireMeshGeometry(const xiiMeshResourceHandle& hMesh);
  /// Releases one reference acquired through AcquireMeshGeometry(). The residency record remains
  /// valid until uiFrameIndex has completed on the GPU.
  static void ReleaseMeshGeometry(const xiiMeshResourceHandle& hMesh, xiiUInt64 uiFrameIndex);

  static void                            RequestResidency(xiiGeometryHandle handle, xiiUInt32 uiMinimumLod, xiiUInt64 uiFrameIndex);
  static void                            Touch(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex);
  static void                            ProcessStreaming(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiUInt64 uiUploadBudgetBytes);

  [[nodiscard]] static bool                               IsValid(xiiGeometryHandle handle);
  [[nodiscard]] static xiiEnum<xiiGeometryResidencyState> GetState(xiiGeometryHandle handle);
  [[nodiscard]] static const xiiGpuGeometryRecord*        GetGpuRecord(xiiGeometryHandle handle);
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer>         GetMetadataBuffer();
  [[nodiscard]] static xiiSharedPtr<xiiGALBuffer>         GetMeshletMetadataBuffer();
  [[nodiscard]] static xiiGeometryResidencyStats          GetStats();

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
    xiiEnum<xiiGeometryResidencyState> m_State           = xiiGeometryResidencyState::Unloaded;
    xiiUInt64                          m_uiLastUsedFrame = 0U;
    xiiUInt64                          m_uiRetireFrame   = 0U;
    xiiUInt64                          m_uiResidentBytes = 0U;
    xiiUInt32                          m_uiGeneration    = 1U;
    xiiUInt32                          m_uiRequestedLod  = 0U;
    bool                               m_bAllocated      = false;
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
