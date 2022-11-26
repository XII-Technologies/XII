#pragma once

#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Shader/Types.h>
#include <memory>

class xiiShaderTransform;

class XII_RENDERERCORE_DLL xiiSkinnedMeshRenderData : public xiiMeshRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkinnedMeshRenderData, xiiMeshRenderData);

public:
  virtual void                FillBatchIdAndSortingKey() override;
  xiiGALBufferHandle          m_hSkinningTransforms;
  xiiArrayPtr<const xiiUInt8> m_pNewSkinningTransformData;
  std::shared_ptr<bool>       m_bTransformsUpdated;
};

struct XII_RENDERERCORE_DLL xiiSkinningState
{
  xiiSkinningState();
  ~xiiSkinningState();

  void Clear();

  /// \brief Holds the current CPU-side copy of the skinning matrices. Modify these and call TransformsChanged() to send them to the GPU.
  xiiDynamicArray<xiiShaderTransform, xiiAlignedAllocatorWrapper> m_Transforms;

  /// \brief Call this, after modifying m_Transforms, to make the renderer apply the update.
  void TransformsChanged();

  void FillSkinnedMeshRenderData(xiiSkinnedMeshRenderData& renderData) const;

private:
  xiiGALBufferHandle    m_hGpuBuffer;
  std::shared_ptr<bool> m_bTransformsUpdated[2];
};
