#pragma once

#include <GraphicsCore/Meshes/MeshRenderer.h>

/// \brief Implements rendering of skinned meshes
class XII_GRAPHICSCORE_DLL xiiSkinnedMeshRenderer : public xiiMeshRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSkinnedMeshRenderer, xiiMeshRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSkinnedMeshRenderer);

public:
  xiiSkinnedMeshRenderer();
  ~xiiSkinnedMeshRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;

protected:
  virtual void SetAdditionalData(const xiiRenderViewContext& renderViewContext, const xiiMeshRenderData* pRenderData) const override;
};
