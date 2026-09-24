/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <GraphicsCore/Meshes/MeshBufferResource.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

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
  xiiUInt32 m_uiIndex = xiiInvalidIndex;
  xiiUInt32 m_uiGeneration = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryHandle);

struct XII_GRAPHICSCORE_DLL xiiGeometryLodSource
{
  xiiMeshBufferResourceHandle m_hMeshBuffer;
  float                       m_fMinimumScreenCoverage = 0.0f;
};

struct XII_GRAPHICSCORE_DLL xiiGeometryDescription
{
  xiiHybridArray<xiiGeometryLodSource, 8U> m_Lods;
  xiiUInt32 m_uiStreamingPriority = 0U;
  bool      m_bPinned = false;
};

/// One LOD consumed by GPU LOD selection and command generation.
struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuGeometryLod
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiVertexBufferIndex = xiiInvalidIndex;
  xiiUInt32 m_uiIndexBufferIndex = xiiInvalidIndex;
  xiiUInt32 m_uiMeshletBufferIndex = xiiInvalidIndex;
  xiiUInt32 m_uiMeshletRemapBufferIndex = xiiInvalidIndex;
  xiiUInt32 m_uiMeshletPrimitiveBufferIndex = xiiInvalidIndex;
  xiiUInt32 m_uiVertexCount = 0U;
  xiiUInt32 m_uiIndexCount = 0U;
  xiiUInt32 m_uiMeshletCount = 0U;
  float     m_fMinimumScreenCoverage = 0.0f;
  xiiUInt32 m_uiIndexType = 0U;
  xiiUInt32 m_uiMeshletMetadataOffset = 0U;
  xiiUInt32 m_uiPadding = 0U;
};

struct XII_GRAPHICSCORE_DLL alignas(16) xiiGpuGeometryRecord
{
  XII_DECLARE_POD_TYPE();
  static constexpr xiiUInt32 s_uiMaxLods = 8U;

  xiiVec4           m_BoundsCenterRadius = xiiVec4::MakeZero();
  xiiVec4           m_BoundsExtents = xiiVec4::MakeZero();
  xiiGpuGeometryLod m_Lods[s_uiMaxLods];
  xiiUInt32         m_uiLodCount = 0U;
  xiiUInt32         m_uiGeneration = 0U;
  xiiUInt32         m_uiResidentLodMask = 0U;
  xiiUInt32         m_uiFlags = 0U;
};

struct XII_GRAPHICSCORE_DLL xiiGeometryResidencyStats
{
  XII_DECLARE_POD_TYPE();
  xiiUInt32 m_uiGeometryCount = 0U;
  xiiUInt32 m_uiResidentGeometryCount = 0U;
  xiiUInt32 m_uiPendingGeometryCount = 0U;
  xiiUInt64 m_uiResidentBytes = 0U;
  xiiUInt64 m_uiBudgetBytes = 0U;
  xiiUInt64 m_uiUploadedBytes = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiGeometryResidencyStats);

/// Owns the stable GPU geometry table and coordinates mesh LOD residency.
class XII_GRAPHICSCORE_DLL xiiGeometryResidencyManager
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGeometryResidencyManager);

public:
  xiiGeometryResidencyManager() = default;
  ~xiiGeometryResidencyManager();

  xiiResult Initialize(xiiGALDevice* pDevice, xiiUInt32 uiMaxGeometries = 65536U, xiiUInt32 uiFramesInFlight = 3U, xiiUInt64 uiBudgetBytes = 512ULL * 1024ULL * 1024ULL, xiiUInt32 uiMaxMeshlets = 1024U * 1024U);
  void Shutdown();

  [[nodiscard]] xiiGeometryHandle RegisterGeometry(const xiiGeometryDescription& description);
  void                               UnregisterGeometry(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex);
  void                               RequestResidency(xiiGeometryHandle handle, xiiUInt32 uiMinimumLod, xiiUInt64 uiFrameIndex);
  void                               Touch(xiiGeometryHandle handle, xiiUInt64 uiFrameIndex);
  void                               ProcessStreaming(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame, xiiUInt64 uiUploadBudgetBytes);

  /// Supplies indices allocated by the backend bindless resource table.
  bool SetBindlessIndices(xiiGeometryHandle handle, xiiUInt32 uiLod, xiiUInt32 uiVertex, xiiUInt32 uiIndex, xiiUInt32 uiMeshlet, xiiUInt32 uiRemap, xiiUInt32 uiPrimitive);

  [[nodiscard]] bool IsValid(xiiGeometryHandle handle) const;
  [[nodiscard]] xiiEnum<xiiGeometryResidencyState> GetState(xiiGeometryHandle handle) const;
  [[nodiscard]] const xiiGpuGeometryRecord* GetGpuRecord(xiiGeometryHandle handle) const;
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetMetadataBuffer() const { return m_pMetadataBuffer; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetMeshletMetadataBuffer() const { return m_pMeshletMetadataBuffer; }
  [[nodiscard]] xiiGeometryResidencyStats GetStats() const;

  struct UploadHandles
  {
    xiiRenderGraphBufferHandle m_hGeometryMetadata;
    xiiRenderGraphBufferHandle m_hMeshletMetadata;
    /// First geometry record in the frame slice uploaded by AddUploadPass().
    xiiUInt32                  m_uiGeometryBaseIndex = 0U;
  };

  [[nodiscard]] UploadHandles AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex);

private:
  struct Slot
  {
    xiiGeometryDescription             m_Description;
    xiiGpuGeometryRecord               m_GpuRecord;
    xiiEnum<xiiGeometryResidencyState> m_State = xiiGeometryResidencyState::Unloaded;
    xiiUInt64                          m_uiLastUsedFrame = 0U;
    xiiUInt64                          m_uiRetireFrame = 0U;
    xiiUInt64                          m_uiResidentBytes = 0U;
    xiiUInt32                          m_uiGeneration = 1U;
    xiiUInt32                          m_uiRequestedLod = 0U;
    bool                               m_bAllocated = false;
    bool                               m_bDirty = false;
    xiiUInt32                          m_uiMeshletArenaOffset[xiiGpuGeometryRecord::s_uiMaxLods] = {};
    xiiUInt32                          m_uiMeshletArenaCount[xiiGpuGeometryRecord::s_uiMaxLods] = {};
  };

  struct Upload
  {
    xiiUInt32 m_uiOffset = 0U;
    xiiGpuGeometryRecord m_Record;
  };

  struct UploadPassData
  {
    xiiRenderGraphBufferHandle m_hGeometryBuffer;
    xiiRenderGraphBufferHandle m_hMeshletBuffer;
    xiiDynamicArray<Upload>    m_Uploads;
    struct MeshletUpload
    {
      xiiUInt32 m_uiOffset = 0U;
      xiiDynamicArray<xiiMeshlet> m_Meshlets;
    };
    xiiDynamicArray<MeshletUpload> m_MeshletUploads;
  };

  struct FreeRange { xiiUInt32 m_uiOffset = 0U; xiiUInt32 m_uiCount = 0U; };

  bool BuildResidentRecord(Slot& slot, xiiUInt64& inout_uiUploadBudget);
  void EnforceBudget(xiiUInt64 uiCompletedFrame);
  bool AllocateMeshlets(xiiUInt32 uiCount, xiiUInt32& out_uiOffset);
  void FreeMeshlets(xiiUInt32 uiOffset, xiiUInt32 uiCount);
  void ReleaseMeshletAllocations(Slot& slot);

  xiiDynamicArray<Slot>      m_Slots;
  xiiDynamicArray<xiiUInt32> m_FreeSlots;
  xiiSharedPtr<xiiGALBuffer> m_pMetadataBuffer;
  xiiSharedPtr<xiiGALBuffer> m_pMeshletMetadataBuffer;
  xiiDynamicArray<FreeRange> m_FreeMeshletRanges;
  xiiDynamicArray<UploadPassData::MeshletUpload> m_PendingMeshletUploads;
  xiiUInt32                  m_uiFramesInFlight = 0U;
  xiiUInt64                  m_uiBudgetBytes = 0U;
  xiiUInt64                  m_uiResidentBytes = 0U;
  xiiUInt64                  m_uiLastUploadedBytes = 0U;
};
