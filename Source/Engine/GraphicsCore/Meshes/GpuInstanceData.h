/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Material/MaterialTypes.h>
#include <GraphicsCore/Meshes/GpuInstanceTypes.h>

class xiiWorld;
class xiiRenderData;
class xiiExtractedRenderData;
class xiiGpuInstanceDataState;

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
  xiiUInt64                                    GetMaximumMeshletCount() const;
  xiiMeshResourceHandle                        GetMeshResource(xiiUInt32 drawIndex) const;
  xiiMaterialGpuHandle                         GetMaterialHandle(xiiUInt32 drawIndex) const;
  /// Returns failure for background, retired candidates, a foreign world's identity or dead owners.
  xiiResult Resolve(xiiUInt32 gpuIdentity, xiiGpuInstanceSource& source) const;
  /// Produces a compact candidate stream for a camera or picking subset; records are not copied.
  void SelectInstances(xiiArrayPtr<xiiRenderData* const> packets, xiiDynamicArray<xiiUInt32>& indices) const;

private:
  xiiUniquePtr<xiiGpuInstanceDataState>      m_pState;
  xiiDynamicArray<xiiGpuSceneInstance>       m_Instances;
  xiiDynamicArray<xiiMat4>                   m_SkinningMatrices;
  xiiDynamicArray<xiiGpuInstanceUploadRange> m_UploadRanges;
};
