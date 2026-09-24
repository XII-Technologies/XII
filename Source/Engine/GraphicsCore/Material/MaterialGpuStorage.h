/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Material/MaterialInstance.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsFoundation/Device/Device.h>

/// Configuration for the global GPU material table. Each frame in flight receives a disjoint
/// slice, so frequent CPU edits never overwrite bytes still consumed by an earlier GPU frame.
struct XII_GRAPHICSCORE_DLL xiiMaterialGpuStorageDescription
{
  xiiUInt32 m_uiMaxMaterials        = 65536U;
  xiiUInt32 m_uiMaxParameterBytes   = 256U;
  xiiUInt32 m_uiFramesInFlight      = 3U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialGpuStorageDescription);

struct XII_GRAPHICSCORE_DLL xiiMaterialGpuStorageStatistics
{
  xiiUInt32 m_uiCapacity           = 0U;
  xiiUInt32 m_uiActiveMaterials    = 0U;
  xiiUInt32 m_uiRetiredMaterials   = 0U;
  xiiUInt32 m_uiLastUploadCount    = 0U;
  xiiUInt64 m_uiLastUploadBytes    = 0U;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiMaterialGpuStorageStatistics);

/// One immutable upload operation captured for a render-graph pass.
struct XII_GRAPHICSCORE_DLL xiiMaterialGpuUpload
{
  xiiMaterialGpuHandle       m_Handle;
  xiiUInt32                  m_uiRevision          = 0U;
  xiiUInt32                  m_uiDestinationOffset = 0U;
  xiiDynamicArray<xiiUInt8>  m_Data;
};

struct XII_GRAPHICSCORE_DLL xiiMaterialGpuUploadBatch
{
  xiiUInt64                              m_uiFrameIndex = 0ULL;
  xiiDynamicArray<xiiMaterialGpuUpload> m_Uploads;

  [[nodiscard]] bool IsEmpty() const { return m_Uploads.IsEmpty(); }
};

/// Central, generation-checked material buffer allocator.
///
/// Material resources no longer allocate one constant buffer each. Instances receive stable slots
/// in a large shader-resource buffer, and only revisions missing from the current frame slice are
/// uploaded. Retired slots are reused only after the caller reports their last-use frame complete.
class XII_GRAPHICSCORE_DLL xiiMaterialGpuStorage
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiMaterialGpuStorage);

public:
  xiiMaterialGpuStorage() = default;
  ~xiiMaterialGpuStorage();

  xiiResult Initialize(xiiGALDevice* pDevice, const xiiMaterialGpuStorageDescription& description = {});
  void      Shutdown();

  [[nodiscard]] xiiMaterialGpuHandle RegisterMaterial(xiiSharedPtr<xiiMaterialInstance> pInstance);
  void                                    UnregisterMaterial(xiiMaterialGpuHandle handle, xiiUInt64 uiFrameIndex);
  void                                    CollectGarbage(xiiUInt64 uiCompletedFrame);

  [[nodiscard]] bool IsValidHandle(xiiMaterialGpuHandle handle) const;
  [[nodiscard]] xiiSharedPtr<xiiMaterialInstance> GetMaterial(xiiMaterialGpuHandle handle) const;
  [[nodiscard]] xiiUInt32 GetGpuOffset(xiiMaterialGpuHandle handle, xiiUInt64 uiFrameIndex) const;
  [[nodiscard]] xiiUInt32 GetMaterialStride() const { return m_uiMaterialStride; }
  [[nodiscard]] xiiSharedPtr<xiiGALBuffer> GetBuffer() const { return m_pBuffer; }
  [[nodiscard]] xiiMaterialGpuStorageStatistics GetStatistics() const;

  /// Captures all revisions required by this frame without mutating the live instances.
  void GatherUploads(xiiUInt64 uiFrameIndex, xiiMaterialGpuUploadBatch& out_batch) const;

  /// Registers a transfer-queue pass and returns the shader-readable buffer version. The returned
  /// handle should be declared as a read by passes that consume material data.
  [[nodiscard]] xiiRenderGraphBufferHandle AddUploadPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex);

private:
  struct Slot
  {
    xiiSharedPtr<xiiMaterialInstance> m_pInstance;
    xiiHybridArray<xiiUInt32, 4U>     m_LastUploadedRevision;
    xiiUInt32                         m_uiGeneration = 1U;
  };

  struct RetiredSlot
  {
    xiiUInt32 m_uiSlot         = xiiInvalidIndex;
    xiiUInt64 m_uiLastUseFrame = 0ULL;
  };

  struct UploadPassData
  {
    xiiRenderGraphBufferHandle m_hBuffer;
    xiiMaterialGpuUploadBatch  m_Batch;
    xiiMaterialGpuStorage*     m_pStorage = nullptr;
  };

  void MarkUploadsRecorded(const xiiMaterialGpuUploadBatch& batch);
  bool IsValidHandleLocked(xiiMaterialGpuHandle handle) const;

  mutable xiiMutex                 m_Mutex;
  xiiMaterialGpuStorageDescription m_Description;
  xiiDynamicArray<Slot>             m_Slots;
  xiiDynamicArray<xiiUInt32>        m_FreeSlots;
  xiiDynamicArray<RetiredSlot>      m_RetiredSlots;
  xiiSharedPtr<xiiGALBuffer>        m_pBuffer;
  xiiUInt32                         m_uiMaterialStride = 0U;
  xiiUInt32                         m_uiActiveCount    = 0U;
  xiiUInt32                         m_uiLastUploadCount = 0U;
  xiiUInt64                         m_uiLastUploadBytes = 0ULL;
};
