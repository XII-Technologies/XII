#pragma once

#include <RendererCore/Meshes/MeshRenderer.h>

/// \brief Implements rendering of skinned meshes
class XII_RENDERERCORE_DLL xiiSkinnedMeshRenderer : public xiiMeshRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkinnedMeshRenderer, xiiMeshRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSkinnedMeshRenderer);

public:
  xiiSkinnedMeshRenderer();
  ~xiiSkinnedMeshRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& types) const override;

protected:
  virtual void SetAdditionalData(const xiiRenderViewContext& renderViewContext, const xiiMeshRenderData* pRenderData) const override;

  static xiiUInt32 s_uiSkinningBufferUpdates;
};
