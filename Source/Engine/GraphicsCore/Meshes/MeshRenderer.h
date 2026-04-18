#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiMeshRenderData;
struct xiiPerInstanceData;

/// \brief Implements rendering of static meshes
class XII_GRAPHICSCORE_DLL xiiMeshRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiMeshRenderer);

public:
  xiiMeshRenderer();
  ~xiiMeshRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

protected:
  virtual void SetAdditionalData(const xiiRenderViewContext& renderViewContext, const xiiMeshRenderData* pRenderData) const;
  virtual void FillPerInstanceData(xiiArrayPtr<xiiPerInstanceData> pInstanceData, const xiiRenderDataBatch& batch, xiiUInt32 uiStartIndex, xiiUInt32& out_uiFilteredCount) const;
};
