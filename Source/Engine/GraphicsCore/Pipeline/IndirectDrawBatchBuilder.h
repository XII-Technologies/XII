#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Mat4.h>

// ============================================================
//  GPU-side per-instance record
// ============================================================

/// \brief One entry in the per-frame GPU instance structured buffer.
///
/// This is the layout consumed by the vertex shader / mesh shader when
/// fetching instance data via a bindless structured buffer SRV.
/// Kept at 128 bytes to fit cleanly inside a cache line.
struct XII_GRAPHICSCORE_DLL alignas(16) xiiGPUInstanceData
{
  xiiMat4   m_LocalToWorld;          ///< Full 4×4 world transform (columns-major).
  xiiMat4   m_LocalToWorldPrev;      ///< Previous frame transform for motion vectors.

  xiiUInt32 m_uiMaterialIndex;       ///< Index into the per-frame material parameter table.
  xiiUInt32 m_uiEntityID;            ///< Entity identifier for segmentation pass.
  xiiUInt32 m_uiMeshletBaseOffset;   ///< First meshlet index in the global meshlet buffer (for meshlet path).
  xiiUInt32 m_uiMeshletCount;        ///< Number of meshlets for this instance.

  xiiUInt32 m_uiLODLevel;            ///< Currently selected LOD level.
  float     m_fScreenCoverage;       ///< Screen-space coverage at last CPU LOD decision.
  xiiUInt32 m_uiFlags;               ///< xiiMeshletRenderFlags packed as uint.
  xiiUInt32 m_uiPad;

  xiiVec3   m_vAABBMin;              ///< Object-space AABB minimum (for GPU-side culling).
  float     m_fPad0;
  xiiVec3   m_vAABBMax;              ///< Object-space AABB maximum.
  float     m_fPad1;
};
XII_CHECK_AT_COMPILETIME(sizeof(xiiGPUInstanceData) == 192);

/// \brief Indirect draw argument layout (matches D3D12_DRAW_INDEXED_ARGUMENTS / VkDrawIndexedIndirectCommand).
struct XII_GRAPHICSCORE_DLL xiiIndirectDrawArgs
{
  xiiUInt32 m_uiIndexCount;
  xiiUInt32 m_uiInstanceCount;
  xiiUInt32 m_uiStartIndex;
  xiiInt32  m_iBaseVertex;
  xiiUInt32 m_uiStartInstance;
};
XII_CHECK_AT_COMPILETIME(sizeof(xiiIndirectDrawArgs) == 20);

/// \brief Indirect dispatch (mesh-shader) argument layout.
struct XII_GRAPHICSCORE_DLL xiiIndirectDispatchMeshArgs
{
  xiiUInt32 m_uiThreadGroupCountX; ///< Number of task shader groups (1 per meshlet batch of 32).
  xiiUInt32 m_uiThreadGroupCountY;
  xiiUInt32 m_uiThreadGroupCountZ;
  xiiUInt32 m_uiStartInstance;
  xiiUInt32 m_uiInstanceCount;
};

// ============================================================
//  Per-material draw batch
// ============================================================

/// \brief One contiguous block of instances sharing the same material and vertex/index buffer.
struct XII_GRAPHICSCORE_DLL xiiIndirectDrawBatch
{
  xiiMaterialResourceHandle m_hMaterial;
  xiiMeshResourceHandle     m_hMesh;        ///< Null for meshlet batches.
  xiiMeshletResourceHandle  m_hMeshlets;    ///< Null for triangle-list batches.
  bool                      m_bUseMeshlets; ///< True → DispatchMesh, False → DrawIndexedInstanced.

  xiiUInt32 m_uiFirstInstance;             ///< Offset into the per-frame instance buffer.
  xiiUInt32 m_uiInstanceCount;             ///< Number of instances in this batch.
  xiiUInt32 m_uiFirstDrawArg;             ///< Offset into the GPU indirect draw/dispatch buffer.
};

// ============================================================
//  Builder class
// ============================================================

/// \brief CPU-side aggregator that collects xiiStaticMeshRenderData and xiiMeshletRenderData
///        submissions during the extraction phase and compacts them into per-material batches.
///
/// Called once per frame from the render pipeline before GPU culling.
/// Output:
///   - m_InstanceBuffer  → uploaded to GPU as a structured buffer SRV
///   - m_DrawBatches     → used to record indirect draw/dispatch commands
class XII_GRAPHICSCORE_DLL xiiIndirectDrawBatchBuilder
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiIndirectDrawBatchBuilder);

public:
  xiiIndirectDrawBatchBuilder();
  ~xiiIndirectDrawBatchBuilder();

  /// \brief Resets all state. Must be called at the start of each frame.
  void Reset();

  /// \brief Adds one static-mesh instance to the batch.
  void AddStaticMeshInstance(const xiiGPUInstanceData& instance,
                             const xiiMeshResourceHandle& hMesh,
                             const xiiMaterialResourceHandle& hMaterial);

  /// \brief Adds one meshlet instance to the batch.
  void AddMeshletInstance(const xiiGPUInstanceData& instance,
                          const xiiMeshletResourceHandle& hMeshlets,
                          const xiiMaterialResourceHandle& hMaterial);

  /// \brief Finalises all pending batches. Must be called after all Add* calls.
  void Finalise();

  // ---- Accessors ----
  xiiArrayPtr<const xiiGPUInstanceData>     GetInstances()   const { return m_Instances; }
  xiiArrayPtr<const xiiIndirectDrawBatch>   GetBatches()     const { return m_Batches; }
  xiiUInt32                                  GetInstanceCount()const { return m_Instances.GetCount(); }

  /// \brief GPU buffer handle for the instance data (valid after Upload()).
  xiiGALBufferHandle GetInstanceBuffer()     const { return m_hInstanceBuffer; }
  xiiGALBufferHandle GetIndirectArgsBuffer() const { return m_hIndirectArgsBuffer; }

  /// \brief Uploads CPU instance data to the GPU instance structured buffer.
  void Upload(xiiGALCommandList& cmdList);

private:
  struct PendingEntry
  {
    xiiGPUInstanceData        m_Instance;
    xiiMeshResourceHandle     m_hMesh;
    xiiMeshletResourceHandle  m_hMeshlets;
    xiiMaterialResourceHandle m_hMaterial;
    bool                      m_bMeshlet;
  };

  void SortAndBatch();

  xiiDynamicArray<PendingEntry>        m_Pending;
  xiiDynamicArray<xiiGPUInstanceData>  m_Instances;
  xiiDynamicArray<xiiIndirectDrawBatch>m_Batches;

  xiiGALBufferHandle m_hInstanceBuffer;
  xiiGALBufferHandle m_hIndirectArgsBuffer;

  bool m_bFinalised = false;
};
