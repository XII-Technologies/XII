#pragma once

#include <GraphicsCore/Meshes/MeshComponentBase.h>
#include <GraphicsFoundation/Shader/Types.h>

class xiiShaderTransform;

class XII_GRAPHICSCORE_DLL xiiSkinnedMeshRenderData : public xiiMeshRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkinnedMeshRenderData, xiiMeshRenderData);

public:
  virtual bool CanBatch(const xiiRenderData& other) const override { return false; }

  xiiSharedPtr<xiiGALBuffer> m_pSkinningTransforms;
};

struct XII_GRAPHICSCORE_DLL xiiSkinningState
{
  xiiSkinningState();
  ~xiiSkinningState();

  void Clear();

  /// \brief Holds the current CPU-side copy of the skinning matrices. Modify these and call TransformsChanged() to send them to the GPU.
  xiiDynamicArray<xiiShaderTransform, xiiAlignedAllocatorWrapper> m_Transforms;

  /// \brief Call this, after modifying m_Transforms, to make the renderer apply the update.
  void TransformsChanged();

  xiiSharedPtr<xiiGALBuffer> m_pGpuBuffer;
};
