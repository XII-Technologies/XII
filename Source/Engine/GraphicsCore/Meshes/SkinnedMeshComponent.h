#pragma once

#include <GraphicsCore/Meshes/MeshComponentBase.h>
#include <GraphicsFoundation/Shader/Types.h>
#include <memory>

class xiiShaderTransform;

class XII_GRAPHICSCORE_DLL xiiSkinnedMeshRenderData : public xiiMeshRenderData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkinnedMeshRenderData, xiiMeshRenderData);

public:
  virtual bool CanBatch(const xiiRenderData& other) const override { return false; }

  xiiGALBufferHandle m_hSkinningTransforms;
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

  xiiGALBufferHandle m_hGpuBuffer;
};
