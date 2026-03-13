#pragma once

#include <GraphicsCore/Pipeline/Renderer.h>

class xiiMeshRenderData;
struct xiiPerInstanceData;

class XII_GRAPHICSCORE_DLL xiiMeshRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMeshRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiMeshRenderer);

public:
  xiiMeshRenderer();
  ~xiiMeshRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;
};
