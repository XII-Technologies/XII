/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Material/MaterialTypes.h>
#include <GraphicsCore/Meshes/GpuInstanceTypes.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>

class xiiWorld;
class xiiRenderData;
class xiiExtractedRenderData;
class xiiGpuInstanceDataState;
struct xiiMeshMorphTarget;

struct xiiGpuInstanceUploadStatistics
{
  xiiUInt64 m_uiBytesUploaded = 0;
  xiiUInt32 m_uiUploadRanges  = 0;
  xiiUInt32 m_uiUploadPasses  = 0;
};

/// Packed, derived GPU draw data for one world. Has no object authoring, hierarchy or transform
/// propagation API. Only extraction changes membership; world/component handles remain authoritative.
/// Update and borrowed reads require the owning world's CPU extraction boundary. Graph callbacks
/// copy their required upload bytes so they never access mutable world/component storage.
class XII_GRAPHICSCORE_DLL xiiGpuInstanceData
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiGpuInstanceData);

public:
  explicit xiiGpuInstanceData(const xiiWorld& world);
  ~xiiGpuInstanceData();

  xiiResult                                    UpdateFromExtraction(xiiArrayPtr<xiiRenderData* const> packets, xiiUInt64 frame);
  xiiResult                                    UpdateFromExtraction(const xiiExtractedRenderData& packets, xiiUInt64 frame);
  xiiArrayPtr<const xiiGpuSceneInstance>       GetInstances() const { return m_Instances; }
  xiiArrayPtr<const xiiMat4>                   GetSkinningMatrices() const { return m_SkinningMatrices; }
  xiiArrayPtr<const xiiGpuInstanceUploadRange> GetUploadRanges() const { return m_UploadRanges; }
  xiiUInt32                                    GetInstanceCount() const { return m_Instances.GetCount(); }
  /// Changes when packed records, membership or pose bytes change; never a serialized identity.
  xiiUInt64                             GetDataRevision() const { return m_uiDataRevision; }
  xiiUInt64                             GetMaximumMeshletCount() const;
  xiiMeshResourceHandle                 GetMeshResource(xiiUInt32 drawIndex) const;
  xiiMaterialGpuHandle                  GetMaterialHandle(xiiUInt32 drawIndex) const;
  xiiArrayPtr<const float>              GetMorphWeights(xiiUInt32 drawIndex) const;
  xiiMeshBufferResourceHandle           GetMeshBufferResource(xiiUInt32 drawIndex) const;
  xiiArrayPtr<const xiiMeshMorphTarget> GetMorphTargets(xiiUInt32 drawIndex) const;
  /// Returns failure for background, retired candidates, a foreign world's identity or dead owners.
  xiiResult Resolve(xiiUInt32 gpuIdentity, xiiGpuInstanceSource& source) const;
  /// Produces a compact candidate stream for a camera or picking subset; records are not copied.
  void SelectInstances(xiiArrayPtr<xiiRenderData* const> packets, xiiDynamicArray<xiiUInt32>& indices) const;

  struct UploadHandles
  {
    xiiRenderGraphBufferHandle m_Instances;
    xiiRenderGraphBufferHandle m_Skinning;
  };
  /// Shared world allocations, imported by explicit graph handles. Dirty bytes are copied into
  /// graph-owned upload data; later cameras/occurrences reuse the same frame's physical buffers.
  UploadHandles                  AddUploadPass(xiiRenderGraph& graph, xiiUInt64 frame) const;
  xiiGpuInstanceUploadStatistics GetUploadStatistics() const;

private:
  xiiUInt64                                  m_uiDataRevision = 0;
  xiiUniquePtr<xiiGpuInstanceDataState>      m_pState;
  xiiDynamicArray<xiiGpuSceneInstance>       m_Instances;
  xiiDynamicArray<xiiMat4>                   m_SkinningMatrices;
  xiiDynamicArray<xiiGpuInstanceUploadRange> m_UploadRanges;
};
